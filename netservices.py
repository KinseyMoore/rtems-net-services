# SPDX-License-Identifier: BSD-2-Clause

#  Copyright (C) 2022 On-Line Applications Research Corporation (OAR)
#  Written by Kinsey Moore <kinsey.moore@oarcorp.com>
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#  1. Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
#  2. Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
#  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
#  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#  POSSIBILITY OF SUCH DAMAGE.

import json
import os

import rtems_waf.rtems as rtems

def removeprefix(data, prefix):
    if data.startswith(prefix):
        return data[len(prefix):]
    return data


def options(opt):
    copts = opt.option_groups['configure options']
    copts.add_option('--optimization',
                     default='-O2',
                     dest='optimization',
                     help='Optimaization level (default: %default)')


def add_flags(flags, new_flags):
    for flag in new_flags:
        if flag not in flags:
            flags.append(flag)


def check_net_lib(conf, lib, name):
    net_name = 'NET_' + name.upper()
    conf.check_cc(lib=lib,
                  ldflags=['-lrtemsdefaultconfig'],
                  uselib_store=net_name, mandatory=False)
    if 'LIB_' + net_name in conf.env:
        conf.env.STACK_NAME = name
        return True
    return False


def bsp_configure(conf, arch_bsp):
    conf.env.OPTIMIZATION = conf.options.optimization
    conf.env.LIB += ['m']

    section_flags = ["-fdata-sections", "-ffunction-sections"]
    add_flags(conf.env.CFLAGS, section_flags)
    add_flags(conf.env.CXXFLAGS, section_flags)

    stacks = [check_net_lib(conf, 'bsd', 'libbsd'),
              check_net_lib(conf, 'networking', 'legacy'),
              check_net_lib(conf, 'lwip', 'lwip')]
    stack_count = stacks.count(True)
    if stack_count == 0:
        conf.fatal('No networking stack found')
    if stack_count != 1:
        conf.fatal('More than one networking stack found')

def build(bld):
    stack_name = bld.env.STACK_NAME

    stack_def = 'RTEMS_NET_' + stack_name.upper()
    stack_inc = str(bld.path.find_node('stack/' + stack_name + '/include'))

    ns_cflags = ['-g', '-Wall', bld.env.OPTIMIZATION]

    ntp_source_files = []
    ntp_incl = [stack_inc]

    arch_lib_path = rtems.arch_bsp_lib_path(bld.env.RTEMS_VERSION,
                                            bld.env.RTEMS_ARCH_BSP)

    with open('ntp-file-import.json', 'r') as cf:
        files = json.load(cf)
        for f in files['source-files-to-import']:
            ntp_source_files.append(os.path.join('./bsd', f))
        for f in files['header-paths-to-import']:
            ntp_incl.append(os.path.join('./bsd', f))

    bld.stlib(features='c',
              target='ntp',
              source=ntp_source_files,
              includes=ntp_incl + [stack_inc + '/ntp'],
              cflags=ns_cflags,
              defines=[stack_def, 'HAVE_CONFIG_H=1'],
              use=[stack_name])
    bld.install_files("${PREFIX}/" + arch_lib_path, ["libntp.a"])

    ttcp_incl = [stack_inc, 'ttcp/include']

    ttcp_source_files = ['ttcp/ttcp.c']

    bld.stlib(features='c',
              target='ttcp',
              source=ttcp_source_files,
              includes=ttcp_incl,
              cflags=ns_cflags,
              defines=[stack_def],
              use=[stack_name])
    bld.install_files("${PREFIX}/" + arch_lib_path, ["libttcp.a"])

    def install_headers(root_path):
        for root, dirs, files in os.walk(root_path):
            for name in files:
                ext = os.path.splitext(name)[1]
                src_root = os.path.split(root)
                path = os.path.join(src_root[0], src_root[1])
                if ext == '.h':
                    subpath = removeprefix(removeprefix(path, root_path), "/")
                    bld.install_files(
                        os.path.join("${PREFIX}",
                                     arch_lib_path,
                                     "include",
                                     subpath),
                        os.path.join(path, name)
                    )

    [install_headers(path) for path in ntp_incl]

    #lib_path = os.path.join(bld.env.PREFIX, arch_lib_path)
    #bld.read_stlib('lwip', paths=[lib_path])
    #bld.read_stlib('rtemstest', paths=[lib_path])
    #bld.read_stlib('telnetd', paths=[lib_path])

    libs = ['m', 'rtemstest']

    ntp_test_incl = ntp_incl + ['testsuites']

    bld.program(features='c',
                target='ntp01.exe',
                source='testsuites/ntp01/test_main.c',
                cflags=ns_cflags,
                includes=ntp_test_incl,
                defines=[stack_def],
                lib=libs,
                use=['ntp', stack_name])

    ttcp_test_incl = ttcp_incl + ['testsuites']

    bld.program(features='c',
                target='ttcpshell01.exe',
                source='testsuites/ttcpshell01/test_main.c',
                cflags=ns_cflags,
                defines=[stack_def],
                includes=ttcp_test_incl,
                lib=libs,
                use=['ttcp',  stack_name])

    test_app_incl = [stack_inc, 'testsuites']

    bld.program(features='c',
                target='telnetd01.exe',
                source='testsuites/telnetd01/init.c',
                cflags=ns_cflags,
                defines=[stack_def],
                includes=test_app_incl,
                lib=libs,
                use=['telnetd', stack_name])

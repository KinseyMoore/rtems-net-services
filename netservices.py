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


def net_check_libbsd(conf):
    pass


def net_check_legacy(conf):
    if conf.env.NET_NAME != 'legacy':
        return
    #
    # BSPs must define:
    #  - RTEMS_BSP_NETWORK_DRIVER_NAME
    #  - RTEMS_BSP_NETWORK_DRIVER_ATTACH
    #
    for define in [
            'RTEMS_BSP_NETWORK_DRIVER_NAME', 'RTEMS_BSP_NETWORK_DRIVER_ATTACH'
    ]:
        code = ['#include <bspopts.h>']
        code += ['#include <bsp.h>']
        code += ['#ifndef %s' % (define)]
        code += ['  #error %s not defined' % (define)]
        code += ['#endif']
        try:
            conf.check_cc(fragment=rtems.test_application(code),
                          execute=False,
                          msg='Checking for %s' % (define))
        except conf.errors.WafError:
            conf.fatal(ab + ' does not provide %s' % (define))


def net_check_lwip(conf):
    pass


def net_config_header(bld):
    if not os.path.exists(bld.env.NET_CONFIG):
        bld.fatal('network configuraiton \'%s\' not found' %
                  (bld.env.NET_CONFIG))
    net_mandatory_tags = [
        'NET_CFG_IFACE',
        'NET_CFG_BOOT_PROT',
    ]
    net_optional_tags = [
        'NET_CFG_IFACE_OPTS', 'NET_CFG_SELF_IP', 'NET_CFG_NETMASK',
        'NET_CFG_MAC_ADDR', 'NET_CFG_GATEWAY_IP', 'NET_CFG_DOMAINNAME',
        'NET_CFG_DNS_IP', 'NET_CFG_NTP_IP'
    ]
    try:
        net_cfg_lines = open(bld.env.NET_CONFIG).readlines()
    except:
        bld.fatal('network configuraiton \'%s\' read failed' %
                  (bld.env.NET_CONFIG))
    lc = 0
    sed = 'sed '
    net_defaults = {}
    for l in net_cfg_lines:
        lc += 1
        if not l.strip().startswith('NET_CFG_'):
            bld.fatal('network configuration \'%s\' ' \
                      'invalid config: %d: %s' % (bld.env.NET_CONFIG, lc, l))
        ls = l.strip().split('#', 1)[0]
        if len(ls) == 0:
            continue
        ls = ls.split('=')
        if len(ls) != 2:
            bld.fatal('network configuration \'%s\' ' \
                      'parse error: %d: %s' % (bld.env.NET_CONFIG, lc, l))
        lhs = ls[0].strip()
        rhs = ls[1].strip()
        if lhs in net_mandatory_tags or lhs in net_optional_tags:
            net_defaults[lhs] = rhs
        else:
            bld.fatal('network configuration \'%s\' ' \
                      'invalid config: %d: %s' % (bld.env.NET_CONFIG, lc, l))
    for cfg in net_mandatory_tags:
        if cfg not in net_defaults:
            bld.fatal('network configuration \'%s\' ' \
                      'mandatory config  not found: %s' % (bld.env.NET_CONFIG, cfg))
    for cfg in net_defaults:
        sed += "-e 's/@%s@/%s/' " % (cfg, net_defaults[cfg])
    bld(target=bld.env.NETWORK_CONFIG,
        source='testsuites/include/network-config.h.in',
        rule=sed + ' < ${SRC} > ${TGT}',
        update_outputs=True)


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
    copts.add_option('--net-test-config',
                     default='config.inc',
                     dest='net_config',
                     help='Network test configuration (default: %default)')
    copts.add_option('--ntp-debug',
                     action='store_true',
                     dest='ntp_debug',
                     help='Build NTP with DEBUG enabled (default: %default)')


def add_flags(flags, new_flags):
    for flag in new_flags:
        if flag not in flags:
            flags.append(flag)


def check_net_lib(conf, lib, name):
    net_name = 'NET_' + name.upper()
    conf.check_cc(lib=lib,
                  ldflags=['-lrtemsdefaultconfig'],
                  uselib_store=net_name,
                  mandatory=False)
    if 'LIB_' + net_name in conf.env:
        conf.env.NET_NAME = name
        # clean up the check
        conf.env['LDFLAGS_' + net_name] = []
        conf.env['LIB_' + net_name] += ['m']
        return True
    return False


def bsp_configure(conf, arch_bsp):
    conf.env.NET_CONFIG = conf.options.net_config
    bld_inc = conf.path.get_bld().find_or_declare('include')
    conf.env.NETWORK_CONFIG = str(bld_inc.find_or_declare('network-config.h'))
    conf.env.OPTIMIZATION = conf.options.optimization
    conf.env.LIB += ['m']

    bld_inc = conf.path.get_bld().find_or_declare('include')
    conf.env.IFLAGS = [str(bld_inc)]

    section_flags = ["-fdata-sections", "-ffunction-sections"]
    add_flags(conf.env.CFLAGS, section_flags)
    add_flags(conf.env.CXXFLAGS, section_flags)

    stacks = [
        check_net_lib(conf, 'bsd', 'libbsd'),
        check_net_lib(conf, 'networking', 'legacy'),
        check_net_lib(conf, 'lwip', 'lwip')
    ]
    stack_count = stacks.count(True)
    if stack_count == 0:
        conf.fatal('No networking stack found')
    if stack_count != 1:
        conf.fatal('More than one networking stack found')

    net_check_libbsd(conf)
    net_check_legacy(conf)
    net_check_lwip(conf)

    conf.env.NTP_DEFINES = []
    if conf.options.ntp_debug:
        conf.env.NTP_DEFINES += ['DEBUG=1']


def build(bld):
    net_name = bld.env.NET_NAME
    net_use = 'NET_' + net_name.upper()
    net_def = 'RTEMS_NET_' + net_name.upper() + '=1'
    net_root = os.path.join('net', net_name)
    net_inc = str(bld.path.find_node(os.path.join(net_root, 'include')))
    net_adapter_source = net_root + '/net_adapter.c'

    inc = bld.env.IFLAGS + ['include', net_inc]
    cflags = ['-g', bld.env.OPTIMIZATION]

    ntp_source_files = []
    ntp_incl = inc

    arch_lib_path = rtems.arch_bsp_lib_path(bld.env.RTEMS_VERSION,
                                            bld.env.RTEMS_ARCH_BSP)

    with open('ntp-file-import.json', 'r') as cf:
        files = json.load(cf)
        for f in files['source-files-to-import']:
            ntp_source_files.append(os.path.join('./bsd', f))
        for f in files['header-paths-to-import']:
            ntp_incl.append(os.path.join('./bsd', f))

    net_config_header(bld)

    bld.add_group()

    ntpq_defines = ['NO_MAIN_ALLOWED=1']

    bld.stlib(features='c',
              target='ntp',
              source=ntp_source_files,
              includes=ntp_incl + [os.path.join(net_root, 'ntp')],
              cflags=cflags,
              defines=[net_def, 'HAVE_CONFIG_H=1'] + ntpq_defines +
              bld.env.NTP_DEFINES,
              use=[net_use])
    bld.install_files("${PREFIX}/" + arch_lib_path, ["libntp.a"])
    ntp_rtems_inc = bld.path.find_dir('bsd/rtemsbsd/include')
    if ntp_rtems_inc != None:
        bld.install_files(os.path.join("${PREFIX}", arch_lib_path, "include"),
                          ntp_rtems_inc.ant_glob('**/**.h'),
                          cwd=ntp_rtems_inc,
                          relative_trick=True)

    ttcp_incl = inc + ['ttcp/include']

    ttcp_source_files = ['ttcp/ttcp.c']

    bld.stlib(features='c',
              target='ttcp',
              source=ttcp_source_files,
              includes=ttcp_incl,
              cflags=cflags,
              defines=[net_def],
              use=[net_use])
    bld.install_files("${PREFIX}/" + arch_lib_path, ["libttcp.a"])

    libs = ['rtemstest', 'debugger']

    ntp_test_incl = ntp_incl + ['testsuites']
    ntp_test_sources = ['testsuites/ntp01/test_main.c', net_adapter_source]

    bld.program(features='c',
                target='ntp01.exe',
                source=ntp_test_sources,
                cflags=cflags,
                includes=ntp_test_incl,
                defines=[net_def],
                lib=['telnetd'] + libs,
                use=['ntp', net_use])

    ttcp_test_incl = ttcp_incl + ['testsuites']
    ttcp_test_sources = ['testsuites/ttcpshell01/test_main.c']
    ttcp_test_sources += [net_adapter_source]

    bld.program(features='c',
                target='ttcpshell01.exe',
                source=ttcp_test_sources,
                cflags=cflags,
                defines=[net_def],
                includes=ttcp_test_incl,
                lib=libs,
                use=['ttcp', net_use])

    tlnt_test_incl = inc + ['testsuites']
    tlnt_test_sources = ['testsuites/telnetd01/init.c']
    tlnt_test_sources += [net_adapter_source]

    bld.program(features='c',
                target='telnetd01.exe',
                source=tlnt_test_sources,
                cflags=cflags,
                defines=[net_def],
                includes=tlnt_test_incl,
                lib=['telnetd'] + libs,
                use=[net_use])

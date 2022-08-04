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

from rtems_waf import rtems
import json
import os


def removeprefix(data, prefix):
    if data.startswith(prefix):
        return data[len(prefix):]
    return data


def build(bld):
    ntp_source_files = []
    ntp_incl = []

    arch_lib_path = rtems.arch_bsp_lib_path(bld.env.RTEMS_VERSION,
                                            bld.env.RTEMS_ARCH_BSP)

    with open('ntp-file-import.json', 'r') as cf:
        files = json.load(cf)
        for f in files['source-files-to-import']:
            ntp_source_files.append(os.path.join('./sebhbsd', f))
        for f in files['header-paths-to-import']:
            ntp_incl.append(os.path.join('./sebhbsd', f))

    ntp_obj_incl = []
    ntp_obj_incl.extend(ntp_incl)

    bld(features='c',
        target='ntp_obj',
        cflags='-g -Wall -O0 -DHAVE_CONFIG_H=1',
        includes=' '.join(ntp_obj_incl),
        source=ntp_source_files,
        )

    bld(features='c cstlib',
        target='ntp',
        cflags='-g -Wall -O0 -DHAVE_CONFIG_H=1',
        use=['ntp_obj'])
    bld.install_files("${PREFIX}/" + arch_lib_path, ["libntp.a"])

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

    lib_path = os.path.join(bld.env.PREFIX, arch_lib_path)
    bld.read_stlib('lwip', paths=[lib_path])
    bld.read_stlib('rtemstest', paths=[lib_path])


def add_flags(flags, new_flags):
    for flag in new_flags:
        if flag not in flags:
            flags.append(flag)


def bsp_configure(conf, arch_bsp):
    conf.env.LIB += ['m']
    section_flags = ["-fdata-sections", "-ffunction-sections"]
    add_flags(conf.env.CFLAGS, section_flags)
    add_flags(conf.env.CXXFLAGS, section_flags)

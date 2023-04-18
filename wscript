#!/usr/bin/env python

#
# RTEMS Project (https://www.rtems.org/)
#
# Copyright (c) 2021 Vijay Kumar Banerjee <vijay@rtems.org>.
# All rights reserved.
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
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

from __future__ import print_function

try:
    import configparser
except:
    import ConfigParser as configparser

import netservices
import sys

top = '.'

rtems_version = "6"
subdirs = []


try:
    import rtems_waf.rtems as rtems
except rtems_waf.DoesNotExist:
    print("error: no rtems_waf git submodule; see README.waf")
    sys.exit(1)


def recurse(ctx):
    for sd in subdirs:
        ctx.recurse(sd)

def init(ctx):
    rtems.init(ctx, version=rtems_version, long_commands=True)


def options(opt):
    rtems.options(opt)
    netservices.options(opt)
    recurse(opt)


def no_unicode(value):
    if sys.version_info[0] > 2:
        return value
    if isinstance(value, unicode):
        return str(value)
    return value


def get_config():
    cp = configparser.ConfigParser()
    filename = "config.ini"
    if filename not in cp.read([filename]):
        return None
    return cp


def get_configured_bsps(cp):
    if not cp:
        return "all"
    bsps = []
    for raw_bsp in cp.sections():
        bsps.append(no_unicode(raw_bsp))
    return ",".join(bsps)


def bsp_configure(conf, arch_bsp):
    env = conf.env.derive()
    ab = conf.env.RTEMS_ARCH_BSP
    conf.msg('Configure variant: ', ab)
    conf.setenv(ab, env)
    netservices.bsp_configure(conf, arch_bsp)
    recurse(conf)
    conf.setenv(ab)


def configure(conf):
    cp = get_config()
    if conf.options.rtems_bsps == "all":
        conf.options.rtems_bsps = get_configured_bsps(cp)
    rtems.configure(conf, bsp_configure)


def build(bld):
    rtems.build(bld)
    netservices.build(bld)
    bld.add_group()
    recurse(bld)

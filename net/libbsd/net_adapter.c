/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @brief This file contains the adapter necessary for network services tests to
 *        run on lwIP.
 */

/*
 * COPYRIGHT (c) 2023. On-Line Applications Research Corporation (OAR).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <net_adapter.h>
#include <stddef.h>
#include <assert.h>
#include <sysexits.h>
#include <machine/rtems-bsd-commands.h>
#include <rtems/rtems/status.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <rtems/dhcpcd.h>
#include <rtems/bsd/bsd.h>
#include <rtems/bsd/iface.h>

#define RTEMS_BSD_CONFIG_NET_PF_UNIX
#define RTEMS_BSD_CONFIG_NET_IP_MROUTE

#ifdef RTEMS_BSD_MODULE_NETINET6
#define RTEMS_BSD_CONFIG_NET_IP6_MROUTE
#endif

#define RTEMS_BSD_CONFIG_NET_IF_BRIDGE
#define RTEMS_BSD_CONFIG_NET_IF_LAGG
#define RTEMS_BSD_CONFIG_NET_IF_VLAN
#define RTEMS_BSD_CONFIG_BSP_CONFIG
#define RTEMS_BSD_CONFIG_INIT
#include <machine/rtems-bsd-config.h>

static void
default_network_ifconfig_hwif0(char *ifname)
{
  int exit_code;
  char *ifcfg[] = {
    "ifconfig",
    ifname,
    "up",
    NULL
  };

  exit_code = rtems_bsd_command_ifconfig(RTEMS_BSD_ARGC(ifcfg), ifcfg);
  assert(exit_code == EX_OK);
}

static void
default_network_dhcpcd(void)
{
  static const char default_cfg[] = "clientid libbsd test client\n";
  rtems_status_code sc;
  int fd;
  int rv;
  ssize_t n;

  fd = open("/etc/dhcpcd.conf", O_CREAT | O_WRONLY,
      S_IRWXU | S_IRWXG | S_IRWXO);
  assert(fd >= 0);

  n = write(fd, default_cfg, sizeof(default_cfg) - 1);
  assert(n == (ssize_t) sizeof(default_cfg) - 1);

  rv = close(fd);
  assert(rv == 0);

  sc = rtems_dhcpcd_start(NULL);
  assert(sc == RTEMS_SUCCESSFUL);
}

static void
default_network_set_self_prio(rtems_task_priority prio)
{
  rtems_status_code sc;

  sc = rtems_task_set_priority(RTEMS_SELF, prio, &prio);
  assert(sc == RTEMS_SUCCESSFUL);
}

int net_start(void)
{
  char *ifname;
  char ifnamebuf[IF_NAMESIZE];
  rtems_status_code sc;

  /*
   * Default the syslog priority to 'debug' to aid developers.
   */
  rtems_bsd_setlogpriority("debug");

  /* Let other tasks run to complete background work */
  default_network_set_self_prio(RTEMS_MAXIMUM_PRIORITY - 1U);

  rtems_bsd_initialize();

  ifname = if_indextoname(1, &ifnamebuf[0]);

  /* Let the callout timer allocate its resources */
  sc = rtems_task_wake_after(2);
  assert(sc == RTEMS_SUCCESSFUL);

  rtems_bsd_ifconfig_lo0();
  default_network_ifconfig_hwif0(ifname);
  default_network_dhcpcd();

  // needs to wait for DHCP to finish
  return 0;
}

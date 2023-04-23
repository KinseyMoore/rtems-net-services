/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @brief This file contains the adapter necessary for network services tests to
 *        run on lwIP.
 */

/*
 * Copyright (c) 2023. Chris Johns (chris@contemporary.software)
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

#include <stdio.h>

#include <rtems.h>
#include <rtems/dhcp.h>
#include <rtems/rtems_bsdnet.h>

#include <bsp.h>

#include <net_adapter.h>

#include <network-config.h>

static char* iface = NET_CFG_IFACE;
static char* boot_prot = NET_CFG_BOOT_PROT;

static struct rtems_bsdnet_ifconfig ifcfg = {
  RTEMS_BSP_NETWORK_DRIVER_NAME,
  RTEMS_BSP_NETWORK_DRIVER_ATTACH
};

struct rtems_bsdnet_config rtems_bsdnet_config;

static bool rtems_net_legacy_config(struct rtems_bsdnet_config* bsd) {
  if (bsd->ifconfig == NULL) {
    bsd->ifconfig = &ifcfg;
  }
  ifcfg.name = iface;
#ifdef NET_CFG_SELF_IP
  ifcfg.ip_address = NET_CFG_SELF_IP;
#endif
#ifdef NET_CFG_NETMASK
  ifcfg.ip_netmask = NET_CFG_NETMASK;
#endif
#ifdef NET_CFG_GATEWAY_IP
  bsd->gateway = NET_CFG_GATEWAY_IP;
#endif
#ifdef NET_CFG_DOMAINNAME
  bsd->domainname = NET_CFG_DOMAINNAME;
#endif
#ifdef NET_CFG_DNS_IP
  bsd->name_server[0] = NET_CFG_DNS_IP;
#endif
  if (strcmp(boot_prot, "static") == 0) {
    bsd->bootp = NULL;
  } else if (strcmp(boot_prot, "bootp") == 0) {
    bsd->bootp = rtems_bsdnet_do_bootp;
  } else if (strcmp(boot_prot, "dhcp") == 0) {
    bsd->bootp = rtems_bsdnet_do_dhcp;
  } else {
    printf("%s: %d: invalid network configuration: %s\n",
           __FILE__, __LINE__, boot_prot);
    return false;
  }
  return true;
}

int net_start(void) {
  int rv;
  rv = rtems_net_legacy_config(&rtems_bsdnet_config);
  if (!rv) {
    return -1;
  }
  rv = rtems_bsdnet_initialize_network();
  if (rv != 0) {
    printf("error: legacy stack initialization failed\n");
    return -1;
  }
  return 0;
}

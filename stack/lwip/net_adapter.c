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

#include <lwip/dhcp.h>
#include <lwip/prot/dhcp.h>
#include <arch/sys_arch.h>

#include <netstart.h>
#include <bsd_compat.h>
#include <net_adapter.h>

struct netif net_interface;

int net_start()
{
  ip_addr_t ipaddr, netmask, gw;

  IP_ADDR4( &ipaddr, 10, 0, 2, 14 );
  IP_ADDR4( &netmask, 255, 255, 255, 0 );
  IP_ADDR4( &gw, 10, 0, 2, 3 );
  unsigned char mac_ethernet_address[] = { 0x00, 0x0a, 0x35, 0x00, 0x22, 0x01 };

  rtems_bsd_compat_initialize();

  int ret = start_networking(
    &net_interface,
    &ipaddr,
    &netmask,
    &gw,
    mac_ethernet_address
  );

  if ( ret != 0 ) {
    return ret;
  }

  dhcp_start( &net_interface );

  /*
   * Wait for DHCP bind to start NTP. lwIP does automatic address updating in
   * the backend that NTP isn't prepared for which causes socket conflicts when
   * the socket for the old address gets updated to the new address and NTP's
   * address information for the old socket is stale. NTP tries to create a new
   * socket for the new address before deleting the old one and gets an error
   * because it can't bind twice to the same address. This causes NTP
   * acquisition to be delayed by minutes in the worst case.
   */
  volatile struct dhcp *dhcp_state = netif_dhcp_data(&net_interface);
  if ( dhcp_state != NULL ) {
    while (dhcp_state->state != DHCP_STATE_BOUND) {
      sleep(1);
    }
  }
  return 0;
}

/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @brief RTEMS shell is started with DHCP client. It is possible to run TTCP.
 */

/*
 * COPYRIGHT (c) 2021. On-Line Applications Research Corporation (OAR).
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

#include <rtems.h>
#include <rtems/shell.h>
#include <rtems/console.h>
#include <ttcp.h>

#if RTEMS_NET_LWIP
#include <lwip/dhcp.h>
#include <arch/sys_arch.h>
#include <netstart.h>
#endif

#include <tmacros.h>

const char rtems_test_name[] = "lwIP TTCP 1";

static int net_start(void);

#if RTEMS_NET_LWIP
struct netif net_interface;

rtems_shell_cmd_t shell_TTCP_Command = {
  "ttcp",                                           /* name */
  "ttcp -h                     # to get help",      /* usage */
  "net",                                            /* topic */
  rtems_shell_main_ttcp,                            /* command */
  NULL,                                             /* alias */
  NULL                                              /* next */
};

static int net_start(void) {
  ip_addr_t ipaddr, netmask, gw;

  IP_ADDR4( &ipaddr, 10, 0, 2, 14 );
  IP_ADDR4( &netmask, 255, 255, 255, 0 );
  IP_ADDR4( &gw, 10, 0, 2, 3 );
  unsigned char mac_ethernet_address[] = { 0x00, 0x0a, 0x35, 0x00, 0x22, 0x01 };

  ret = start_networking(
    &net_interface,
    &ipaddr,
    &netmask,
    &gw,
    mac_ethernet_address
  );

  if ( ret != 0 ) {
    return 1;
  }

  dhcp_start( &net_interface );

  return 0;
}

static rtems_task Init( rtems_task_argument argument )
{
  rtems_status_code sc;
  int ret;

  TEST_BEGIN();

  rtems_test_assert( net_start() == 0 );

  rtems_shell_init_environment();

  sc = rtems_shell_init(
    "SHLL",                       /* task name */
    RTEMS_MINIMUM_STACK_SIZE * 4, /* task stack size */
    100,                          /* task priority */
    "/dev/console",               /* device name */
    false,                        /* run forever */
    true,                         /* wait for shell to terminate */
    NULL                          /* login check function,
                                     use NULL to disable a login check */
  );
  rtems_test_assert( sc == RTEMS_SUCCESSFUL );
  sys_arch_delay( 300000 );

  TEST_END();
  rtems_test_exit( 0 );
}

#define CONFIGURE_INIT

#define CONFIGURE_MICROSECONDS_PER_TICK 10000

#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK

#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 32

#define CONFIGURE_SHELL_COMMANDS_INIT
#define CONFIGURE_SHELL_COMMANDS_ALL
#define CONFIGURE_SHELL_USER_COMMANDS &shell_TTCP_Command

#define CONFIGURE_MAXIMUM_TASKS 12

#define CONFIGURE_MAXIMUM_POSIX_KEYS 1
#define CONFIGURE_MAXIMUM_SEMAPHORES 20
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES 10

#define CONFIGURE_INITIAL_EXTENSIONS RTEMS_TEST_INITIAL_EXTENSION

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#include <rtems/shellconfig.h>

#include <rtems/confdefs.h>

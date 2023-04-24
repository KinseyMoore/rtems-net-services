/**
 * @file
 *
 * @brief Code used to test the NTP deamon.
 */

/*
 * Copyright (C) 2022 embedded brains GmbH (http://www.embedded-brains.de)
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>

#include <rtems/console.h>
#include <rtems/imfs.h>
#include <rtems/ntpd.h>
#include <rtems/shell.h>

#include <rtems/shellconfig-net-services.h>

#include <net_adapter.h>
#include <net_adapter_extra.h>
#include <network-config.h>

#include <rtems/telnetd.h>

#include <tmacros.h>

const char rtems_test_name[] = "NTP 1";

rtems_shell_env_t env;

static void telnet_shell( char *name, void *arg )
{
  rtems_shell_dup_current_env( &env );

  env.devname = name;
  env.taskname = "NTPD";

  rtems_shell_main_loop( &env );
}

rtems_telnetd_config_table rtems_telnetd_config = {
  .command = telnet_shell,
  .stack_size = 8 * RTEMS_MINIMUM_STACK_SIZE,
};

#define NTP_DEBUG 0
#define ntp_xstr(s) ntp_str(s)
#define ntp_str(s) #s
#define NTP_DEBUG_STR ntp_xstr(NTP_DEBUG)

static const char etc_resolv_conf[] =
#ifdef NET_CFG_NTP_IP
    "nameserver " NET_CFG_DNS_IP "\n";
#else
    "nameserver 8.8.8.8\n";
#endif

static const char etc_ntp_conf[] =
    "tos minclock 3 maxclock 6\n"
#ifdef NET_CFG_NTP_IP
    "server " NET_CFG_NTP_IP "\n"
#else
    "pool 0.freebsd.pool.ntp.org iburst\n"
#endif
    "restrict default limited kod nomodify notrap noquery nopeer\n"
    "restrict source  limited kod nomodify notrap noquery\n"
    "restrict 10.0.0.0 mask 255.0.0.0\n"
    "restrict 172.16.0.0 mask 255.240.0.0\n"
    "restrict 192.168.0.0 mask 255.255.0.0\n"
    "restrict 127.0.0.1\n"
    "restrict ::1\n"
    "leapfile \"/etc/leap-seconds\"\n";

static const char etc_leap_seconds[] =
    "#\n"
    "#	In the following text, the symbol '#' introduces\n"
    "#	a comment, which continues from that symbol until\n"
    "#	the end of the line. A plain comment line has a\n"
    "#	whitespace character following the comment indicator.\n"
    "#	There are also special comment lines defined below.\n"
    "#	A special comment will always have a non-whitespace\n"
    "#	character in column 2.\n"
    "#\n"
    "#	A blank line should be ignored.\n"
    "#\n"
    "#	The following table shows the corrections that must\n"
    "#	be applied to compute International Atomic Time (TAI)\n"
    "#	from the Coordinated Universal Time (UTC) values that\n"
    "#	are transmitted by almost all time services.\n"
    "#\n"
    "#	The first column shows an epoch as a number of seconds\n"
    "#	since 1 January 1900, 00:00:00 (1900.0 is also used to\n"
    "#	indicate the same epoch.) Both of these time stamp formats\n"
    "#	ignore the complexities of the time scales that were\n"
    "#	used before the current definition of UTC at the start\n"
    "#	of 1972. (See note 3 below.)\n"
    "#	The second column shows the number of seconds that\n"
    "#	must be added to UTC to compute TAI for any timestamp\n"
    "#	at or after that epoch. The value on each line is\n"
    "#	valid from the indicated initial instant until the\n"
    "#	epoch given on the next one or indefinitely into the\n"
    "#	future if there is no next line.\n"
    "#	(The comment on each line shows the representation of\n"
    "#	the corresponding initial epoch in the usual\n"
    "#	day-month-year format. The epoch always begins at\n"
    "#	00:00:00 UTC on the indicated day. See Note 5 below.)\n"
    "#\n"
    "#	Important notes:\n"
    "#\n"
    "#	1. Coordinated Universal Time (UTC) is often referred to\n"
    "#	as Greenwich Mean Time (GMT). The GMT time scale is no\n"
    "#	longer used, and the use of GMT to designate UTC is\n"
    "#	discouraged.\n"
    "#\n"
    "#	2. The UTC time scale is realized by many national\n"
    "#	laboratories and timing centers. Each laboratory\n"
    "#	identifies its realization with its name: Thus\n"
    "#	UTC(NIST), UTC(USNO), etc. The differences among\n"
    "#	these different realizations are typically on the\n"
    "#	order of a few nanoseconds (i.e., 0.000 000 00x s)\n"
    "#	and can be ignored for many purposes. These differences\n"
    "#	are tabulated in Circular T, which is published monthly\n"
    "#	by the International Bureau of Weights and Measures\n"
    "#	(BIPM). See www.bipm.org for more information.\n"
    "#\n"
    "#	3. The current definition of the relationship between UTC\n"
    "#	and TAI dates from 1 January 1972. A number of different\n"
    "#	time scales were in use before that epoch, and it can be\n"
    "#	quite difficult to compute precise timestamps and time\n"
    "#	intervals in those \"prehistoric\" days. For more information,\n"
    "#	consult:\n"
    "#\n"
    "#		The Explanatory Supplement to the Astronomical\n"
    "#		Ephemeris.\n"
    "#	or\n"
    "#		Terry Quinn, \"The BIPM and the Accurate Measurement\n"
    "#		of Time,\" Proc. of the IEEE, Vol. 79, pp. 894-905,\n"
    "#		July, 1991. <http://dx.doi.org/10.1109/5.84965>\n"
    "#		reprinted in:\n"
    "#		   Christine Hackman and Donald B Sullivan (eds.)\n"
    "#		   Time and Frequency Measurement\n"
    "#		   American Association of Physics Teachers (1996)\n"
    "#		   <http://tf.nist.gov/general/pdf/1168.pdf>, pp. 75-86\n"
    "#\n"
    "#	4. The decision to insert a leap second into UTC is currently\n"
    "#	the responsibility of the International Earth Rotation and\n"
    "#	Reference Systems Service. (The name was changed from the\n"
    "#	International Earth Rotation Service, but the acronym IERS\n"
    "#	is still used.)\n"
    "#\n"
    "#	Leap seconds are announced by the IERS in its Bulletin C.\n"
    "#\n"
    "#	See www.iers.org for more details.\n"
    "#\n"
    "#	Every national laboratory and timing center uses the\n"
    "#	data from the BIPM and the IERS to construct UTC(lab),\n"
    "#	their local realization of UTC.\n"
    "#\n"
    "#	Although the definition also includes the possibility\n"
    "#	of dropping seconds (\"negative\" leap seconds), this has\n"
    "#	never been done and is unlikely to be necessary in the\n"
    "#	foreseeable future.\n"
    "#\n"
    "#	5. If your system keeps time as the number of seconds since\n"
    "#	some epoch (e.g., NTP timestamps), then the algorithm for\n"
    "#	assigning a UTC time stamp to an event that happens during a positive\n"
    "#	leap second is not well defined. The official name of that leap\n"
    "#	second is 23:59:60, but there is no way of representing that time\n"
    "#	in these systems.\n"
    "#	Many systems of this type effectively stop the system clock for\n"
    "#	one second during the leap second and use a time that is equivalent\n"
    "#	to 23:59:59 UTC twice. For these systems, the corresponding TAI\n"
    "#	timestamp would be obtained by advancing to the next entry in the\n"
    "#	following table when the time equivalent to 23:59:59 UTC\n"
    "#	is used for the second time. Thus the leap second which\n"
    "#	occurred on 30 June 1972 at 23:59:59 UTC would have TAI\n"
    "#	timestamps computed as follows:\n"
    "#\n"
    "#	...\n"
    "#	30 June 1972 23:59:59 (2287785599, first time):	TAI= UTC + 10 seconds\n"
    "#	30 June 1972 23:59:60 (2287785599,second time):	TAI= UTC + 11 seconds\n"
    "#	1  July 1972 00:00:00 (2287785600)		TAI= UTC + 11 seconds\n"
    "#	...\n"
    "#\n"
    "#	If your system realizes the leap second by repeating 00:00:00 UTC twice\n"
    "#	(this is possible but not usual), then the advance to the next entry\n"
    "#	in the table must occur the second time that a time equivalent to\n"
    "#	00:00:00 UTC is used. Thus, using the same example as above:\n"
    "#\n"
    "#	...\n"
    "#       30 June 1972 23:59:59 (2287785599):		TAI= UTC + 10 seconds\n"
    "#       30 June 1972 23:59:60 (2287785600, first time):	TAI= UTC + 10 seconds\n"
    "#       1  July 1972 00:00:00 (2287785600,second time):	TAI= UTC + 11 seconds\n"
    "#	...\n"
    "#\n"
    "#	in both cases the use of timestamps based on TAI produces a smooth\n"
    "#	time scale with no discontinuity in the time interval. However,\n"
    "#	although the long-term behavior of the time scale is correct in both\n"
    "#	methods, the second method is technically not correct because it adds\n"
    "#	the extra second to the wrong day.\n"
    "#\n"
    "#	This complexity would not be needed for negative leap seconds (if they\n"
    "#	are ever used). The UTC time would skip 23:59:59 and advance from\n"
    "#	23:59:58 to 00:00:00 in that case. The TAI offset would decrease by\n"
    "#	1 second at the same instant. This is a much easier situation to deal\n"
    "#	with, since the difficulty of unambiguously representing the epoch\n"
    "#	during the leap second does not arise.\n"
    "#\n"
    "#	Some systems implement leap seconds by amortizing the leap second\n"
    "#	over the last few minutes of the day. The frequency of the local\n"
    "#	clock is decreased (or increased) to realize the positive (or\n"
    "#	negative) leap second. This method removes the time step described\n"
    "#	above. Although the long-term behavior of the time scale is correct\n"
    "#	in this case, this method introduces an error during the adjustment\n"
    "#	period both in time and in frequency with respect to the official\n"
    "#	definition of UTC.\n"
    "#\n"
    "#	Questions or comments to:\n"
    "#		Judah Levine\n"
    "#		Time and Frequency Division\n"
    "#		NIST\n"
    "#		Boulder, Colorado\n"
    "#		Judah.Levine@nist.gov\n"
    "#\n"
    "#	Last Update of leap second values:   8 July 2016\n"
    "#\n"
    "#	The following line shows this last update date in NTP timestamp\n"
    "#	format. This is the date on which the most recent change to\n"
    "#	the leap second data was added to the file. This line can\n"
    "#	be identified by the unique pair of characters in the first two\n"
    "#	columns as shown below.\n"
    "#\n"
    "#$	 3676924800\n"
    "#\n"
    "#	The NTP timestamps are in units of seconds since the NTP epoch,\n"
    "#	which is 1 January 1900, 00:00:00. The Modified Julian Day number\n"
    "#	corresponding to the NTP time stamp, X, can be computed as\n"
    "#\n"
    "#	X/86400 + 15020\n"
    "#\n"
    "#	where the first term converts seconds to days and the second\n"
    "#	term adds the MJD corresponding to the time origin defined above.\n"
    "#	The integer portion of the result is the integer MJD for that\n"
    "#	day, and any remainder is the time of day, expressed as the\n"
    "#	fraction of the day since 0 hours UTC. The conversion from day\n"
    "#	fraction to seconds or to hours, minutes, and seconds may involve\n"
    "#	rounding or truncation, depending on the method used in the\n"
    "#	computation.\n"
    "#\n"
    "#	The data in this file will be updated periodically as new leap\n"
    "#	seconds are announced. In addition to being entered on the line\n"
    "#	above, the update time (in NTP format) will be added to the basic\n"
    "#	file name leap-seconds to form the name leap-seconds.<NTP TIME>.\n"
    "#	In addition, the generic name leap-seconds.list will always point to\n"
    "#	the most recent version of the file.\n"
    "#\n"
    "#	This update procedure will be performed only when a new leap second\n"
    "#	is announced.\n"
    "#\n"
    "#	The following entry specifies the expiration date of the data\n"
    "#	in this file in units of seconds since the origin at the instant\n"
    "#	1 January 1900, 00:00:00. This expiration date will be changed\n"
    "#	at least twice per year whether or not a new leap second is\n"
    "#	announced. These semi-annual changes will be made no later\n"
    "#	than 1 June and 1 December of each year to indicate what\n"
    "#	action (if any) is to be taken on 30 June and 31 December,\n"
    "#	respectively. (These are the customary effective dates for new\n"
    "#	leap seconds.) This expiration date will be identified by a\n"
    "#	unique pair of characters in columns 1 and 2 as shown below.\n"
    "#	In the unlikely event that a leap second is announced with an\n"
    "#	effective date other than 30 June or 31 December, then this\n"
    "#	file will be edited to include that leap second as soon as it is\n"
    "#	announced or at least one month before the effective date\n"
    "#	(whichever is later).\n"
    "#	If an announcement by the IERS specifies that no leap second is\n"
    "#	scheduled, then only the expiration date of the file will\n"
    "#	be advanced to show that the information in the file is still\n"
    "#	current -- the update time stamp, the data and the name of the file\n"
    "#	will not change.\n"
    "#\n"
    "#	Updated through IERS Bulletin C62\n"
    "#	File expires on:  28 June 2022\n"
    "#\n"
    "#@	3865363200\n"
    "#\n"
    "2272060800	10	# 1 Jan 1972\n"
    "2287785600	11	# 1 Jul 1972\n"
    "2303683200	12	# 1 Jan 1973\n"
    "2335219200	13	# 1 Jan 1974\n"
    "2366755200	14	# 1 Jan 1975\n"
    "2398291200	15	# 1 Jan 1976\n"
    "2429913600	16	# 1 Jan 1977\n"
    "2461449600	17	# 1 Jan 1978\n"
    "2492985600	18	# 1 Jan 1979\n"
    "2524521600	19	# 1 Jan 1980\n"
    "2571782400	20	# 1 Jul 1981\n"
    "2603318400	21	# 1 Jul 1982\n"
    "2634854400	22	# 1 Jul 1983\n"
    "2698012800	23	# 1 Jul 1985\n"
    "2776982400	24	# 1 Jan 1988\n"
    "2840140800	25	# 1 Jan 1990\n"
    "2871676800	26	# 1 Jan 1991\n"
    "2918937600	27	# 1 Jul 1992\n"
    "2950473600	28	# 1 Jul 1993\n"
    "2982009600	29	# 1 Jul 1994\n"
    "3029443200	30	# 1 Jan 1996\n"
    "3076704000	31	# 1 Jul 1997\n"
    "3124137600	32	# 1 Jan 1999\n"
    "3345062400	33	# 1 Jan 2006\n"
    "3439756800	34	# 1 Jan 2009\n"
    "3550089600	35	# 1 Jul 2012\n"
    "3644697600	36	# 1 Jul 2015\n"
    "3692217600	37	# 1 Jan 2017\n"
    "#\n"
    "#	the following special comment contains the\n"
    "#	hash value of the data in this file computed\n"
    "#	use the secure hash algorithm as specified\n"
    "#	by FIPS 180-1. See the files in ~/pub/sha for\n"
    "#	the details of how this hash value is\n"
    "#	computed. Note that the hash computation\n"
    "#	ignores comments and whitespace characters\n"
    "#	in data lines. It includes the NTP values\n"
    "#	of both the last modification time and the\n"
    "#	expiration time of the file, but not the\n"
    "#	white space on those lines.\n"
    "#	the hash line is also ignored in the\n"
    "#	computation.\n"
    "#\n"
    "#h 	599d45bf accd4b4f 8b60e46 49b623 7d13b825\n";

static const char etc_services[] =
    "ntp                123/tcp      # Network Time Protocol  [Dave_Mills] [RFC5905]\n"
    "ntp                123/udp      # Network Time Protocol  [Dave_Mills] [RFC5905]\n";

static bool ntp_finished;
static rtems_id ntpd_id;

static void setup_etc(void)
{
  int rv;

  /* FIXME: no direct IMFS */

  rv = IMFS_make_linearfile("/etc/resolv.conf", S_IWUSR | S_IRUSR |
      S_IRGRP | S_IROTH, etc_resolv_conf, sizeof(etc_resolv_conf));
  assert(rv == 0);

  rv = IMFS_make_linearfile("/etc/ntp.conf", S_IWUSR | S_IRUSR |
      S_IRGRP | S_IROTH, etc_ntp_conf, sizeof(etc_ntp_conf));
  assert(rv == 0);

  rv = IMFS_make_linearfile("/etc/leap-seconds", S_IWUSR | S_IRUSR |
      S_IRGRP | S_IROTH, etc_leap_seconds, sizeof(etc_leap_seconds));
  assert(rv == 0);

  rv = IMFS_make_linearfile("/etc/services", S_IWUSR | S_IRUSR |
      S_IRGRP | S_IROTH, etc_services, sizeof(etc_services));
  assert(rv == 0);

}

static rtems_task ntpd_runner(
  rtems_task_argument argument
)
{
  char *argv[] = {
    "ntpd",
    "-g",
#if NTP_DEBUG
    "--set-debug-level=" NTP_DEBUG_STR,
#endif
    NULL
  };
  const int argc = ((sizeof(argv) / sizeof(argv[0])) - 1);

  (void)rtems_ntpd_run(argc, argv);
  ntp_finished = true;
}

static void run_test(void)
{
  rtems_status_code sc;
  char *argv[] = {
    "ntpq",
    "127.0.0.1",
    NULL
  };
  const int argc = ((sizeof(argv) / sizeof(argv[0])) - 1);

  setup_etc();

  rtems_shell_add_cmd_struct(&rtems_shell_NTPQ_Command);

  sc = rtems_telnetd_start( &rtems_telnetd_config );
  rtems_test_assert( sc == RTEMS_SUCCESSFUL );

  sc = rtems_shell_init("SHLL", 16 * 1024, 1, CONSOLE_DEVICE_NAME,
    false, false, NULL);
  directive_failed( sc, "rtems_shell_init" );
  assert(sc == RTEMS_SUCCESSFUL);

  sc = rtems_task_create(
    rtems_build_name( 'n', 't', 'p', 'd' ),
    10,
    8 * 1024,
    RTEMS_TIMESLICE,
    RTEMS_FLOATING_POINT,
    &ntpd_id
  );
  directive_failed( sc, "rtems_task_create" );
  sc = rtems_task_start( ntpd_id, ntpd_runner, 0 );
  directive_failed( sc, "rtems_task_start of TA1" );

  while (!ntp_finished) {
    sleep(2);
  }
}

static rtems_task Init( rtems_task_argument argument )
{
  rtems_printer test_printer;
  rtems_print_printer_printf(&test_printer);
  rtems_test_printer = test_printer;

  TEST_BEGIN();
  fflush(stdout);
  sleep(1);

  rtems_test_assert( net_start() == 0 );

  rtems_shell_init_environment();

  run_test();

  TEST_END();
  fflush(stdout);

  rtems_test_exit( 0 );
}

#define CONFIGURE_MICROSECONDS_PER_TICK 1000

#define CONFIGURE_SHELL_COMMANDS_INIT
#define CONFIGURE_SHELL_COMMANDS_ALL

#include <bsp/irq-info.h>

#define CONFIGURE_SHELL_USER_COMMANDS \
  CONFIGURE_SHELL_USER_COMMANDS_ADAPTER, \
  &rtems_shell_DATE_Command, \
  &rtems_shell_SHUTDOWN_Command

#define CONFIGURE_SHELL_COMMAND_CPUINFO
#define CONFIGURE_SHELL_COMMAND_CPUUSE
#define CONFIGURE_SHELL_COMMAND_PERIODUSE
#define CONFIGURE_SHELL_COMMAND_STACKUSE
#define CONFIGURE_SHELL_COMMAND_PROFREPORT
#define CONFIGURE_SHELL_COMMAND_RTC
#if RTEMS_NET_LEGACY
  #define RTEMS_NETWORKING 1
  #define CONFIGURE_SHELL_COMMANDS_ALL_NETWORKING
#endif /* RTEMS_NET_LEGACY */

#include <rtems/shellconfig.h>

#define CONFIGURE_INIT
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_LIBBLOCK
#define CONFIGURE_APPLICATION_NEEDS_STUB_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_ZERO_DRIVER

#define CONFIGURE_MAXIMUM_DRIVERS 32
#define CONFIGURE_MAXIMUM_FILE_DESCRIPTORS 64

#define CONFIGURE_MAXIMUM_USER_EXTENSIONS 1

#define CONFIGURE_UNLIMITED_ALLOCATION_SIZE 32

#define CONFIGURE_BDBUF_BUFFER_MAX_SIZE (64 * 1024)
#define CONFIGURE_BDBUF_MAX_READ_AHEAD_BLOCKS 4
#define CONFIGURE_BDBUF_CACHE_MEMORY_SIZE (1 * 1024 * 1024)

#define CONFIGURE_INIT_TASK_STACK_SIZE (32 * 1024)

#define CONFIGURE_MAXIMUM_TASKS 25

#define CONFIGURE_MAXIMUM_USER_EXTENSIONS 1

#define CONFIGURE_MAXIMUM_POSIX_KEYS 1
#define CONFIGURE_MAXIMUM_SEMAPHORES 20
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES 10

#define CONFIGURE_INITIAL_EXTENSIONS RTEMS_TEST_INITIAL_EXTENSION

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE

#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_FLOATING_POINT

#define CONFIGURE_UNLIMITED_OBJECTS
#define CONFIGURE_UNIFIED_WORK_AREAS

#include <rtems/confdefs.h>

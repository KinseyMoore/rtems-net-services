/**
 * @file
 *
 * @ingroup rtems_bsd_rtems
 *
 * @brief NTPQ command
 */

/*
 * Copyright (c) 2023 Chris Johns <chrisj@rtems.org>.  All rights reserved.
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

/*
 * Copyright (C) 1992-2020 The University of Delaware and Network Time Foundation,
 * All rights reserved.
 * http://ntp.org/license
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appears in all copies and that
 * both the copyright notice and this permission notice appear in supporting
 * documentation, and that the name The University of Delaware not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The University of Delaware and Network
 * Time Foundation makes no representations about the suitability this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * Large pieces of this code have been taken from tnpq and reworked to
 * be something usable on RTEMS.
 */

/*
 * We only have the BSD program main call. Do not override the
 * following.
 */
#define RTEMS_BSD_PROGRAM_NO_ABORT_WRAP
#define RTEMS_BSD_PROGRAM_NO_PRINTF_WRAP
#define RTEMS_BSD_PROGRAM_NO_OPEN_WRAP
#define RTEMS_BSD_PROGRAM_NO_SOCKET_WRAP
#define RTEMS_BSD_PROGRAM_NO_CLOSE_WRAP
#define RTEMS_BSD_PROGRAM_NO_FOPEN_WRAP
#define RTEMS_BSD_PROGRAM_NO_FCLOSE_WRAP
#define RTEMS_BSD_PROGRAM_NO_MALLOC_WRAP
#define RTEMS_BSD_PROGRAM_NO_CALLOC_WRAP
#define RTEMS_BSD_PROGRAM_NO_REALLOC_WRAP
#define RTEMS_BSD_PROGRAM_NO_REALLOC_WRAP
#define RTEMS_BSD_PROGRAM_NO_STRDUP_WRAP
#define RTEMS_BSD_PROGRAM_NO_STRNDUP_WRAP
#define RTEMS_BSD_PROGRAM_NO_VASPRINTF_WRAP
#define RTEMS_BSD_PROGRAM_NO_ASPRINTF_WRAP
#define RTEMS_BSD_PROGRAM_NO_FREE_WRAP

#include <config.h>

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>

#include <rtems.h>
#include <rtems/thread.h>
#include <machine/rtems-bsd-program.h>

#include <rtems/ntpq.h>
#include <ntpq.h>
#define LIBNTPQ_C 1
#include <libntpq.h>

#include <rtems/shellconfig-net-services.h>
#include <rtems/libio_.h>

/*
 * NTPQ is sequential
 */
static rtems_recursive_mutex ntpq_lock = RTEMS_RECURSIVE_MUTEX_INITIALIZER("ntpq");

/*
 * NTPQ is full of globals and so we protect usage with single mutex
 * from the user interface. There is no easy way to make ntpq thread
 * safe without a lot of changes and the BSD command support is not
 * fully ported to net seervices.
 */
fd_set* rtems_ntpq_fd_set;
 size_t rtems_ntpq_fd_set_size;
static int rtems_ntpq_error_value;
static char rtems_ntpq_error_str[128];
static FILE* rtems_ntpq_outputfp;
static char* rtems_ntpq_output_buf;
static size_t rtems_ntpq_output_buf_size;

/**
 * SSL support stubs
 */
const char *keytype_name(int nid) {
  (void) nid;
  return "MD5";
}

int keytype_from_text(const char *text, size_t *pdigest_len) {
  (void) text;
  (void) pdigest_len;
  return NID_md5;
}

char *getpass_keytype(int keytype) {
  (void) keytype;
  return "\0";
}

void rtems_ntpq_verror(int error_code, const char* format, va_list ap) {
  size_t len = 6;
  rtems_ntpq_error_value = error_code;
  strcpy(rtems_ntpq_error_str, "ntpq: ");
  len += vsnprintf(
    rtems_ntpq_error_str + 6, sizeof(rtems_ntpq_error_str) - 7, format, ap);
  snprintf(
    rtems_ntpq_error_str + len, sizeof(rtems_ntpq_error_str) - len - 1,
    ": %d: %s", errno, strerror(errno));
}

void rtems_ntpq_error(int error_code, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  rtems_ntpq_verror(error_code, format, ap);
  va_end(ap);
}

void rtems_ntpq_error_msg(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  rtems_ntpq_error(-1, format, ap);
  va_end(ap);
}

/*
 * Default values we use.
 */
#define	DEFHOST		"localhost"	/* default host name */
#define	DEFTIMEOUT	5		/* wait 5 seconds for 1st pkt */
#define	DEFSTIMEOUT	3		/* and 3 more for each additional */
/*
 * Requests are automatically retried once, so total timeout with no
 * response is a bit over 2 * DEFTIMEOUT, or 10 seconds.  At the other
 * extreme, a request eliciting 32 packets of responses each for some
 * reason nearly DEFSTIMEOUT seconds after the prior in that series,
 * with a single packet dropped, would take around 32 * DEFSTIMEOUT, or
 * 93 seconds to fail each of two times, or 186 seconds.
 * Some commands involve a series of requests, such as "peers" and
 * "mrulist", so the cumulative timeouts are even longer for those.
 */
#define	DEFDELAY	0x51EB852	/* 20 milliseconds, l_fp fraction */
#define	LENHOSTNAME	256		/* host name is 256 characters long */

extern int always_auth;
extern int ai_fam_templ;
extern int ai_fam_default;
extern int wideremote;
extern int rawmode;
extern struct servent *server_entry;
extern struct association *assoc_cache;
extern u_char pktversion;
extern SOCKET sockfd;
extern int havehost;
extern int s_port;
extern struct servent *server_entry;
extern int sequence;
extern struct sock_timeval tvout;
extern struct sock_timeval tvsout;
extern l_fp delay_time;
extern char currenthost[LENHOSTNAME];
extern int currenthostisnum;
extern struct sockaddr_in hostaddr;
extern int showhostnames;
extern int wideremote;

static void rtems_ntpq_init(void) {
  const struct sock_timeval tvout_ = { DEFTIMEOUT, 0 };
  const struct sock_timeval tvsout_ = { DEFSTIMEOUT, 0 };
  if (sockfd > 0) {
    close(sockfd);
    havehost = 0;
  }
  sockfd = -1;
  havehost = 0;
  s_port = 0;
  server_entry = NULL;
  sequence = 0;

  old_rv = 1;
  drefid = -1;
  always_auth = 0;
  rawmode = 0;
  pktversion = NTP_OLDVERSION + 1;
  tvout = tvout_;
  tvsout = tvsout_;
  memset(&delay_time, 0, sizeof(delay_time));
  memset(currenthost, 0, LENHOSTNAME);
  currenthostisnum = 0;
  memset(&hostaddr, 0, sizeof(hostaddr));
  showhostnames = 1;
  wideremote = 0;
  ai_fam_templ = 0;
  ai_fam_default = 0;

  if (assoc_cache != NULL) {
    free(assoc_cache);
  }
  assoc_cache = NULL;
  assoc_cache_slots = 0;
  numassoc = 0;

  numhosts = 0;
}

int rtems_ntpq_create(size_t output_buf_size) {
  size_t alloc_size;
  size_t fd_set_break;
  rtems_recursive_mutex_lock(&ntpq_lock);
  if (rtems_ntpq_output_buf != NULL) {
    rtems_ntpq_error(EEXIST, "already open");
    rtems_recursive_mutex_unlock(&ntpq_lock);
    return -1;
  }
  rtems_ntpq_output_buf_size = output_buf_size;
  alloc_size = output_buf_size;
  if ((alloc_size & 16) != 0) {
    alloc_size = ((alloc_size / 16) + 1) * 16;
  }
  fd_set_break = alloc_size;
  rtems_ntpq_fd_set_size =
    sizeof(fd_set) * (howmany(rtems_libio_number_iops, sizeof(fd_set) * 8));
  alloc_size += rtems_ntpq_fd_set_size;
  rtems_ntpq_output_buf = calloc(1, alloc_size);
  if (rtems_ntpq_output_buf == NULL) {
    rtems_ntpq_fd_set_size = 0;
    rtems_ntpq_error(ENOMEM, "no memory");
    rtems_recursive_mutex_unlock(&ntpq_lock);
    return -1;
  }
  rtems_ntpq_fd_set = (fd_set*) (rtems_ntpq_output_buf + fd_set_break);
  rtems_ntpq_outputfp = fopen("/dev/null", "wb");
  if (rtems_ntpq_outputfp == NULL) {
    rtems_ntpq_error(errno, "buffered file pointer");
    free(rtems_ntpq_output_buf);
    rtems_ntpq_fd_set_size = 0;
    rtems_ntpq_output_buf_size = 0;
    rtems_ntpq_output_buf = NULL;
    rtems_ntpq_fd_set = NULL;
    rtems_recursive_mutex_unlock(&ntpq_lock);
    return -1;
  }
  setbuffer(
    rtems_ntpq_outputfp, &rtems_ntpq_output_buf[0],
    rtems_ntpq_output_buf_size);
  rtems_ntpq_init();
  rtems_recursive_mutex_unlock(&ntpq_lock);
  return 0;
}

void rtems_ntpq_destroy(void) {
  rtems_recursive_mutex_lock(&ntpq_lock);
  if (rtems_ntpq_output_buf != NULL) {
    if (rtems_ntpq_outputfp != NULL) {
      fclose(rtems_ntpq_outputfp);
    }
    free(rtems_ntpq_output_buf);
    rtems_ntpq_fd_set_size = 0;
    rtems_ntpq_output_buf_size = 0;
    rtems_ntpq_output_buf = NULL;
    rtems_ntpq_fd_set = NULL;
  }
  rtems_ntpq_init();
  rtems_recursive_mutex_unlock(&ntpq_lock);
}

int rtems_ntpq_error_code(void) {
  int v;
  rtems_recursive_mutex_lock(&ntpq_lock);
  v = rtems_ntpq_error_value;
  rtems_recursive_mutex_unlock(&ntpq_lock);
  return v;
}

const char* rtems_ntpq_error_text(void) {
  return rtems_ntpq_error_str;
}

int rtems_ntpq_create_check(void) {
  int r = 1;
  rtems_recursive_mutex_lock(&ntpq_lock);
  if (rtems_ntpq_outputfp == NULL) {
    rtems_ntpq_error_msg("not open");
    r = 0;
  }
  rtems_recursive_mutex_unlock(&ntpq_lock);
  return r;
}

const char* rtems_ntpq_output(void) {
  const char* o;
  rtems_recursive_mutex_lock(&ntpq_lock);
  o = rtems_ntpq_output_buf;
  rtems_recursive_mutex_unlock(&ntpq_lock);
  return o;
}

FILE* rtems_ntpq_stdout(void) {
  FILE* fp;
  rtems_recursive_mutex_lock(&ntpq_lock);
  fp = rtems_ntpq_outputfp;
  rtems_recursive_mutex_unlock(&ntpq_lock);
  return fp;
}

static int rtems_getarg(const char *str, int code, arg_v *argp) {
  extern struct association *assoc_cache;
  unsigned long ul;

  switch (code & ~OPT) {
  case NTP_STR:
    argp->string = str;
    break;

  case NTP_ADD:
    if (!getnetnum(str, &argp->netnum, NULL, 0))
      return 0;
    break;

  case NTP_UINT:
    if ('&' == str[0]) {
      if (!atouint(&str[1], &ul)) {
        rtems_ntpq_error_msg(
          "association index `%s' invalid/undecodable", str);
        return 0;
      }
      if (0 == numassoc) {
        dogetassoc(rtems_ntpq_outputfp);
        if (0 == numassoc) {
        rtems_ntpq_error_msg("no associations found, `%s' unknown", str);
          return 0;
        }
      }
      ul = min(ul, numassoc);
      argp->uval = assoc_cache[ul - 1].assid;
      break;
    }
    if (!atouint(str, &argp->uval)) {
        rtems_ntpq_error_msg("illegal unsigned value %s", str);
      return 0;
    }
    break;

  case NTP_INT:
    if (!atoint(str, &argp->ival)) {
      rtems_ntpq_error_msg("illegal integer value %s", str);
      return 0;
    }
    break;

  case IP_VERSION:
    if (!strcmp("-6", str)) {
      argp->ival = 6;
    } else if (!strcmp("-4", str)) {
      argp->ival = 4;
    } else {
      rtems_ntpq_error_msg("version must be either 4 or 6\n");
      return 0;
    }
    break;
  }

  return 1;
}

int rtems_ntpq_query_main(int argc, char** argv) {
  struct xcmd* cmd = (struct xcmd*) argv[0];
  struct parse* pcmd = (struct parse*) argv[1];
  FILE* outputfp = (FILE*) argv[2];
  cmd->handler(pcmd, outputfp);
  return 0;
}

int rtems_ntpq_query(
  const int argc, const char** argv, char* output, const size_t size) {
  extern struct xcmd builtins[];
  extern struct xcmd opcmds[];
  char* prog_main_argv[] = { NULL, NULL, NULL, NULL };
  struct parse pcmd;
  struct xcmd* cmd;
  const char* keyword;
  size_t keyword_len;
  int args = argc;
  int arg;
  memset(output, 0, size);
  rtems_recursive_mutex_lock(&ntpq_lock);
  if (!rtems_ntpq_create_check()) {
    rtems_recursive_mutex_unlock(&ntpq_lock);
    return -1;
  }
  if (argc < 1) {
    rtems_ntpq_error_msg("no arguments provided");
    rtems_recursive_mutex_unlock(&ntpq_lock);
    return -1;
  }
  fflush(rtems_ntpq_outputfp);
  memset(rtems_ntpq_output_buf, 0, rtems_ntpq_output_buf_size);
  keyword = argv[0];
  args--;
  argv++;
  keyword_len = strlen(keyword);
  for (cmd = builtins; cmd->keyword != NULL; ++cmd) {
    if (strncmp(keyword, cmd->keyword, keyword_len) == 0) {
      break;
    }
  }
  if (cmd->keyword == NULL) {
    for (cmd = opcmds; cmd->keyword != NULL; ++cmd) {
      if (strncmp(keyword, cmd->keyword, keyword_len) == 0) {
        break;
      }
    }
    if (cmd->keyword == NULL) {
      rtems_ntpq_error_msg("command not found: %s", keyword);
      rtems_recursive_mutex_unlock(&ntpq_lock);
      return -1;
    }
  }
  pcmd.keyword = keyword;
  pcmd.nargs = 0;
  for (arg = 0; arg < MAXARGS && cmd->arg[arg] != NO; ++arg) {
    if (arg == args) {
      break;
    }
    if (arg > args) {
      rtems_ntpq_error_msg("not enough options: %s", keyword);
      rtems_recursive_mutex_unlock(&ntpq_lock);
      return -1;
    }
    if (!rtems_getarg(argv[arg], cmd->arg[arg], &pcmd.argval[arg])) {
      rtems_recursive_mutex_unlock(&ntpq_lock);
      return -1;
    }
    ++pcmd.nargs;
  }
  prog_main_argv[0] = (char*) cmd;
  prog_main_argv[1] = (char*) &pcmd;
  prog_main_argv[2] = (char*) rtems_ntpq_outputfp;
  (void) rtems_bsd_program_call_main(
    "ntpq", rtems_ntpq_query_main, 3, prog_main_argv);
  memcpy(output, rtems_ntpq_output_buf, min(size, rtems_ntpq_output_buf_size));
  output[size - 1] = '\0';
  rtems_recursive_mutex_unlock(&ntpq_lock);
  return 0;
}

int rtems_shell_ntpq_command(int argc, char **argv) {
  const size_t size = 2048;
  char* output = NULL;
  int r;
  argc--;
  argv++;
  if (argc < 1) {
    printf("error: no host and commands\n");
    return 1;
  }
  if (strcmp(argv[0], "open") == 0) {
    r = rtems_ntpq_create(4096);
    if (r == 0) {
      printf("ntpq: open");
    }
  } else if (strcmp(argv[0], "close") == 0) {
    rtems_ntpq_destroy();
    printf("ntpq: closed");
    r = 0;
  } else {
    output = malloc(size);
    if (output == NULL) {
      printf("ntpq: no memory for output\n");
    } else {
      r = rtems_ntpq_query(argc, (const char**) argv, output, size);
    }
  }
  if (r == 0) {
    if (output != NULL) {
      const size_t len = strlen(output);
      printf(rtems_ntpq_output());
      if (len > 0 && output[len - 1] != '\n') {
        printf("\n");
      }
    }
  } else {
    printf("%s\n", rtems_ntpq_error_text());
  }
  if (output != NULL) {
    free(output);
  }
  return r;
}

rtems_shell_cmd_t rtems_shell_NTPQ_Command =
{
    "ntpq",
    "[help]",
    "misc",
    rtems_shell_ntpq_command,
    NULL,
    NULL
};

/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file
 *
 * @ingroup rtems_bsd
 *
 * @brief This header file defines the NTP Queue daemon interfaces.
 */

/*
 * Copyright (C) 2023 Chris Johns <chris@contemporary.software>
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

#ifndef _RTEMS_NTPQ_H
#define _RTEMS_NTPQ_H

#include <ntp_types.h>
#include <ntpq-opts.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Runs the NTP query command (nptq).
 *
 * @param argc is the argument count.
 *
 * @param argv is the vector of arguments.
 *
 * @return This function returns the result.
 */
int rtems_shell_ntpq_command(int argc, char **argv);

int rtems_ntpq_create(size_t output_buf_size);
void rtems_ntpq_destroy(void);

int rtems_ntpq_error_code(void);
const char* rtems_ntpq_error_text(void);
int rtems_ntpq_create_check(void);
const char* rtems_ntpq_output(void);
FILE* rtems_ntpq_stdout(void);
FILE* rtems_ntpq_stderr(void);

#ifdef __cplusplus
}
#endif

#endif /* _RTEMS_NTPQ_H */

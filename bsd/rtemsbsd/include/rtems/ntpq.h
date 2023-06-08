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

#include <inttypes.h>

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

/**
 * @brief Create the NTP query environment
 *
 * Call this function before making a query. It only needs to be
 * called once for the program.
 *
 * @param output_buf_size Size of the output buffer to hold the query
 *
 * @return This function returns the result.
 */
int rtems_ntpq_create(size_t output_buf_size);

/**
 * @brief Destroy the NTP query environment
 *
 * This releases any held resources.
 *
 * @return This function returns the result.
 */
void rtems_ntpq_destroy(void);

/**
 * @brief Query the NTP service
 *
 * Refer to the commands the ntpq command accepts. The output is placed
 * in the provided output buffer.
 *
 * @param argc Argument count
 *
 * @param argv Argument string pointers
 *
 * @param output Buffer to write the output into
 *
 * @param size Size of the putput buffer
 *
 * @return This function returns the result.
 */
int rtems_ntpq_query(const int argc, const char** argv,
		     char* output, const size_t size);

int rtems_ntpq_error_code(void);
const char* rtems_ntpq_error_text(void);
int rtems_ntpq_create_check(void);

#ifdef __cplusplus
}
#endif

#endif /* _RTEMS_NTPQ_H */

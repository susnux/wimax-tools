/*
 * Linux WiMax
 * Test of user space API
 *
 *
 * Copyright (C) 2007-2008 Intel Corporation. All rights reserved.
 * Inaky Perez-Gonzalez <inaky.perez-gonzalez@intel.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * FIXME: docs
 */
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <wimaxll.h>

static
char *status_to_str(int status)
{
	int bytes = 0;
	static char str[64];

	if ((status & 0x1) == WIMAX_RF_OFF)
		bytes += snprintf(str + bytes, sizeof(str),
				  "HW off");
	else
		bytes += snprintf(str + bytes, sizeof(str),
				  "HW on");
	if ((status & 0x2) >> 1 == WIMAX_RF_OFF)
		bytes += snprintf(str + bytes, sizeof(str),
				  " SW off");
	else
		bytes += snprintf(str + bytes, sizeof(str),
				  " SW on");
	return str;
}

int main(int argc, char **argv)
{
	int result, old_status;
	struct wimaxll_handle *wmx;
	char *dev_name;

	if (argc < 2) {
		fprintf(stderr, "E: need an argument "
			"(device interface name)\n");
		return 1;
	}

	dev_name = argv[1];
	wmx = wimaxll_open(dev_name);
	if (wmx == NULL) {
		fprintf(stderr, "E: libwimax: open of interface %s "
			"failed: %m\n", dev_name);
		result = -errno;
		goto error_wimaxll_open;
	}

	fprintf(stderr, "I: querying rfkill status\n");
	result = wimaxll_rfkill(wmx, WIMAX_RF_QUERY);
	if (result < 0) {
		fprintf(stderr, "E: rfkill status query failed: %d\n", result);
		goto error_rfkill_query;
	}
	fprintf(stderr, "I: rfkill status is 0x%x (%s)\n",
		result, status_to_str(result));
	old_status = result;

	fprintf(stderr, "I: Turning ON in software\n");
	result = wimaxll_rfkill(wmx, WIMAX_RF_ON);
	if (result < 0) {
		fprintf(stderr, "E: rfkill sw on failed: %d\n", result);
		goto error_rfkill_query;
	}
	fprintf(stderr, "I: rfkill status is 0x%x (%s)\n", result,
		status_to_str(result));
	if ((result & 0x2) >> 1 != WIMAX_RF_ON) {
		fprintf(stderr, "E: rfkill sw on didn't switch the "
			"radio on: %d\n", result);
		goto error_rfkill_query;
	}

	fprintf(stderr, "I: Turning OFF in software\n");
	result = wimaxll_rfkill(wmx, WIMAX_RF_OFF);
	if (result < 0) {
		fprintf(stderr, "E: rfkill sw-off failed: %d\n", result);
		goto error_rfkill_query;
	}
	fprintf(stderr, "I: rfkill status is 0x%x (%s)\n", result,
		status_to_str(result));
	if ((result & 0x2) >> 1 != WIMAX_RF_OFF) {
		fprintf(stderr, "E: rfkill sw-off didn't switch the "
			"radio off: %d\n", result);
		goto error_rfkill_query;
	}

	fprintf(stderr, "I: toggling rfkill status back\n");
	result = wimaxll_rfkill(wmx, (old_status & 0x2) >> 1 == WIMAX_RF_OFF ?
			      WIMAX_RF_OFF : WIMAX_RF_ON);
	if (result < 0) {
		fprintf(stderr, "E: rfkill toggle failed: %d\n", result);
		goto error_rfkill_query;
	}
	fprintf(stderr, "I: rfkill status is 0x%x (%s)\n",
		result, status_to_str(result));
	if ((result & 0x2) != (old_status & 0x2)) {
		fprintf(stderr, "E: rfkill didn't switch the "
			"radio back to %d: %d\n", old_status, result);
		goto error_rfkill_query;
	}

	result = 0;
error_rfkill_query:
	wimaxll_close(wmx);
error_wimaxll_open:
	return result;
}


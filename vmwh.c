/*
 * Copyright (c) 2010 joshua stein <jcs@jcs.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <unistd.h>

#include "vmwh.h"

void usage(void);

int debug = 0;

int
main(int argc, char *argv[])
{
	int done_init = 0;
	int was_grabbed = 0;
	int handle_mouse = 0;
	int ch;

	x11_verify_xclip_presence();

	while ((ch = getopt(argc, argv, "dm")) != -1)
		switch (ch) {
		case 'd':
			debug = 1;
			break;
		case 'm':
			handle_mouse = 1;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	vmware_check_version();

	vmware_get_mouse_position();

	if (handle_mouse)
		x11_init();

	for (;;) {
		was_grabbed = mouse_grabbed;
		vmware_get_mouse_position();

		if (mouse_grabbed && (!was_grabbed || !done_init)) {
			/* transitioned from host -> guest */
			char *clip = NULL;

			if (debug)
				printf("transitioned from host -> guest\n");

			if (vmware_get_clipboard(&clip)) {
				x11_set_clipboard(clip);
				free(clip);
			}

			if (handle_mouse)
				x11_set_mouse_position(host_mouse_x,
				    host_mouse_y);
		}

		else if (!mouse_grabbed && (was_grabbed || !done_init)) {
			/* transitioned from guest -> host */
			char *clip = NULL;

			if (debug)
				printf("transitioned from guest -> host\n");

			if (x11_get_clipboard(&clip)) {
				vmware_set_clipboard(clip);
				free(clip);
			}

			if (handle_mouse)
				vmware_set_mouse_position(host_mouse_x,
				    host_mouse_y);
		}

		if (!done_init)
			done_init = 1;

		usleep(500);
	}

	return (0);
}

void
usage(void)
{
	(void)fprintf(stderr, "usage: vmwh [-d] [-m]\n");
	exit(1);
}

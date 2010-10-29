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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <vis.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>

#include "vmwh.h"

#define XCLIP_READ "xclip -out -selection clipboard"
#define XCLIP_WRITE "xclip -in -selection clipboard"

struct xinfo {
	Display *dpy;
	int screen;
	Window win;
	Window root;
} x11;

void
x11_init(void) {
	bzero(&x11, sizeof(struct xinfo));

	if (!(x11.dpy = XOpenDisplay(NULL)))
		errx(1, "unable to open display %s", XDisplayName(NULL));
	
	x11.screen = DefaultScreen(x11.dpy);
	x11.root = RootWindow(x11.dpy, x11.screen);

	x11.win = XCreateSimpleWindow(x11.dpy, XDefaultRootWindow(x11.dpy), 0, 0, 1,
		1, 0, 0, 0);

	XSelectInput(x11.dpy, x11.win, PropertyChangeMask);
}

void
x11_set_clipboard(char *buf)
{
	FILE *xclip;

	if (debug) {
		char visbuf[strlen(buf) * 4];
		strnvis(visbuf, buf, sizeof(visbuf), VIS_TAB | VIS_NL | VIS_CSTYLE);
		printf("x11_set_clipboard: \"%s\"\n", visbuf);
	}

	xclip = popen(XCLIP_WRITE, "w");
	if (xclip == NULL) {
		warn("couldn't write to xclip (" XCLIP_WRITE ")");
		return;
	}

	fprintf(xclip, buf);
	fflush(xclip);

	pclose(xclip);
}

int
x11_get_clipboard(char **buf)
{
	FILE *xclip;
	char *tbuf = NULL, *lbuf;
	size_t len;

	xclip = popen(XCLIP_READ, "r");
	if (xclip == NULL) {
		warn("couldn't read from xclip (" XCLIP_READ ")");
		return (0);
	}

	while ((lbuf = fgetln(xclip, &len)) != NULL) {
		lbuf[len] = '\0';

		if (tbuf == NULL) {
			tbuf = malloc(len);
			memcpy(tbuf, lbuf, len);
			tbuf[len] = '\0';
		} else {
			if ((tbuf = realloc(tbuf, strlen(tbuf) + len)) == NULL)
				err(1, "realloc");

			strncat(tbuf, lbuf, len);
		}
	}

	pclose(xclip);

	*buf = tbuf;

	if (debug) {
		if (*buf == NULL)
			printf("x11_get_clipboard: nothing there\n");
		else {
			char visbuf[strlen(*buf) * 4];
			strnvis(visbuf, *buf, sizeof(visbuf), VIS_TAB | VIS_NL | VIS_CSTYLE);
			printf("x11_get_clipboard: \"%s\"\n", visbuf);
		}
	}

	if (*buf == NULL)
		return (0);
	else if (strlen(*buf) == 0) {
		free(*buf);
		return (0);
	} else
		return (1);
}

void
x11_set_cursor(int x, int y)
{
	XWarpPointer(x11.dpy, None, x11.root, 0, 0, 0, 0, x, y);
	XFlush(x11.dpy);
}

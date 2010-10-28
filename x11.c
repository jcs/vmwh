#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <vis.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>

#include "vmwh.h"

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
	XEvent ev, response;
	XSelectionRequestEvent *req;

	if (debug) {
		char visbuf[strlen(buf) * 4];
		strnvis(visbuf, buf, sizeof(visbuf), VIS_WHITE | VIS_CSTYLE);
		printf("x11_set_clipboard: \"%s\"\n", visbuf);
	}

	XSetSelectionOwner(x11.dpy, XA_CLIPBOARD(x11.dpy), x11.win, CurrentTime);

	/* FIXME */ return; 

	while (1) {
		XNextEvent(x11.dpy, &ev);

		if (ev.type != SelectionRequest)
			continue;

		req = &(ev.xselectionrequest);

		if (req->target == XA_STRING) {
			XChangeProperty(x11.dpy, req->requestor,
				req->property, req->target,
				8, PropModeReplace,
				(unsigned char *) buf,
				strlen(buf));
			response.xselection.property = req->property;
		} else
			response.xselection.property = None;

		response.xselection.type = SelectionNotify;
		response.xselection.display = x11.dpy;
		response.xselection.requestor = req->requestor;
		response.xselection.selection = req->selection;
		response.xselection.target = req->target;
		response.xselection.time = req->time;

		XSendEvent(x11.dpy, req->requestor, 0, 0, &response);
		XFlush(x11.dpy);

		break;
	}
}

int
x11_get_clipboard(char **buf)
{
	/* TODO */

	return (0);
}

void
x11_set_cursor(int x, int y)
{
	XWarpPointer(x11.dpy, None, x11.root, 0, 0, 0, 0, x, y);
	XFlush(x11.dpy);
}

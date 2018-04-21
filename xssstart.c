/*
 * Copyright (c) 2016 Vadim Vygonets <vadik@vygo.net>
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

#include <sys/wait.h>

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/extensions/scrnsaver.h>

#ifndef __dead
#ifdef __GNUC__
#define __dead __attribute__((noreturn))
#else
#define __dead
#endif
#endif

static volatile pid_t	 child;
static Display		*dpy;

static void
closedisplay(void)
{
	if (dpy != NULL)
		XCloseDisplay(dpy);
}

static int __dead
eh(Display *dpy, XErrorEvent *e)
{
	char buf[BUFSIZ];

	XGetErrorText(dpy, e->error_code, buf, sizeof(buf));
	errx(1, "X error: %s", buf);
	/* NOTREACHED */
}

static void
sigchld(int sig)
{
	int	 status;

	(void)sig;
	if (wait4(child, &status, WNOHANG, NULL) == child &&
	    (WIFEXITED(status) || WIFSIGNALED(status))) {
		child = 0;
	}
}

int
main(int argc, char *argv[])
{
	XEvent		 ev;
	struct sigaction act = { };
	int		 evbase, errbase, argi, deargi = -1;

	for (argi = 1; argi < argc; argi++) {
		if (strcmp(argv[argi], "--") == 0) {
			deargi = argi + 1;
			argv[argi] = NULL;
		}
	}
	if (argc < 2 || deargi >= argc)
		errx(2, "usage: %s [activate-command [argument ...]] [-- deactivate-command [argument ...]]", argv[0]);
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		errx(1, "no display");
	atexit(closedisplay);
	if (!XScreenSaverQueryExtension(dpy, &evbase, &errbase))
		errx(1, "X11 extension MIT-SCREEN-SAVER not supported");
	XSetErrorHandler(eh);
	XScreenSaverSelectInput(dpy, DefaultRootWindow(dpy),
	    ScreenSaverNotifyMask);
	act.sa_handler = sigchld;
	act.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	if (sigaction(SIGCHLD, &act, NULL) == -1)
		err(1, "sigaction");
	while (XNextEvent(dpy, &ev) == 0) {
		if (child != 0)
			continue;
		switch (((XScreenSaverNotifyEvent *)&ev)->state) {
		case ScreenSaverOn:
			if (deargi == 2)
				continue;
			switch ((child = fork())) {
			case -1:
				err(1, "fork");
			case 0:
				execvp(argv[1], argv + 1);
				dpy = NULL;
				err(1, "exec");
			}
			break;
		case ScreenSaverOff:
			if (deargi < 0)
				continue;
			switch ((child = fork())) {
			case -1:
				err(1, "fork");
			case 0:
				execvp(argv[deargi], argv + deargi);
				dpy = NULL;
				err(1, "exec");
			}
			break;
		}
	}
	return 0;
}

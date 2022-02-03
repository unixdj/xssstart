/*
 * Copyright (c) 2022 Vadim Vygonets <vadik@vygo.net>
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
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
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
static volatile int	 lastsig;
static Display		*dpy;

static void
closedisplay(void)
{
	if (dpy != NULL) {
		XScreenSaverSuspend(dpy, False);
		XCloseDisplay(dpy);
	}
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
handle(int sig)
{
	int	status;

	lastsig = sig;
	if (sig == SIGCHLD &&
	    wait4(child, &status, WNOHANG, NULL) == child &&
	    (WIFEXITED(status) || WIFSIGNALED(status))) {
		child = 0;
	}
}

int
main(int argc, char *argv[])
{
	struct sigaction act = {
		.sa_handler =	handle,
		.sa_flags =	SA_NOCLDSTOP | SA_RESTART,
	};
	sigset_t	set, oldset;
	int		evbase, errbase;

	if ((dpy = XOpenDisplay(NULL)) == NULL)
		errx(1, "no display");
	atexit(closedisplay);
	if (!XScreenSaverQueryExtension(dpy, &evbase, &errbase))
		errx(1, "X11 extension MIT-SCREEN-SAVER not supported");
	XSetErrorHandler(eh);
	XScreenSaverSuspend(dpy, True);
	XFlush(dpy);
	sigemptyset(&set);
	if (sigaction(SIGINT, &act, NULL) == -1)
		err(1, "sigaction");
	if (sigaction(SIGTERM, &act, NULL) == -1)
		err(1, "sigaction");
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGCHLD);
	sigaddset(&set, SIGUSR1);
	sigprocmask(SIG_BLOCK, &set, &oldset);
	if (argc > 1) {
		if (sigaction(SIGCHLD, &act, NULL) == -1)
			err(1, "sigaction");
		if (sigaction(SIGUSR1, &act, NULL) == -1)
			err(1, "sigaction");
		switch ((child = fork())) {
		case -1:
			err(1, "fork");
		case 0:
			execvp(argv[1], argv + 1);
			dpy = NULL;
			{
				int	ee = errno;

				if (kill(getppid(), SIGUSR1) == -1)
					warn("kill");
				errno = ee;
			}
			err(1, "exec");
		}
	}
	for (;;) {
		sigsuspend(&oldset);
		if (lastsig != SIGCHLD || child == 0)
			break;
	}
	if (lastsig == SIGUSR1)
		return 127;
	return 0;
}

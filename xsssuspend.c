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

#define EXIT_RUNTIME	125
#define EXIT_EXEC	126
#define EXIT_ERR	127
#define EXIT_ENOENT	EXIT_ERR
#define EXIT_SIG	128

#define DELAY		3600

static int	 xerr = EXIT_ERR;
static int	 suspended;
static Display	*dpy;

static void
closedisplay(void)
{
	if (dpy != NULL) {
		if (suspended)
			XScreenSaverSuspend(dpy, False);
		XCloseDisplay(dpy);
	}
}

static int __dead
eh(Display *dpy, XErrorEvent *e)
{
	char buf[BUFSIZ];

	XGetErrorText(dpy, e->error_code, buf, sizeof(buf));
	errx(xerr, "X error: %s", buf);
	/* NOTREACHED */
}

int
main(int argc, char *argv[])
{
	struct timespec	ts = {
		.tv_sec		= DELAY,
		.tv_nsec	= 0,
	};
	sigset_t	set;
	pid_t		child = 0;
	int		evbase, errbase, status;

	if ((dpy = XOpenDisplay(NULL)) == NULL)
		errx(EXIT_ERR, "no display");
	atexit(closedisplay);
	XSetErrorHandler(eh);
	if (!XScreenSaverQueryExtension(dpy, &evbase, &errbase))
		errx(EXIT_ERR, "X11 extension MIT-SCREEN-SAVER not supported");
	XScreenSaverSuspend(dpy, True);
	XFlush(dpy);
	suspended = 1;

	sigemptyset(&set);
	sigaddset(&set, SIGTERM);
	if (argc > 1)
		sigaddset(&set, SIGCHLD);
	else
		sigaddset(&set, SIGINT);
	if (sigprocmask(SIG_BLOCK, &set, NULL) == -1)
		err(EXIT_ERR, "sigprocmask");

	if (argc > 1) {
		struct sigaction	sa = {
			.sa_handler	= SIG_IGN,
		};

		if (sigaction(SIGINT, &sa, NULL) == -1)
			err(EXIT_ERR, "sigaction");
		switch ((child = fork())) {
		case -1:
			err(EXIT_ERR, "fork");
		case 0:
			sa.sa_handler = SIG_DFL;
			if (sigaction(SIGINT, &sa, NULL) == -1)
				err(EXIT_ERR, "sigaction");
			if (sigprocmask(SIG_UNBLOCK, &set, NULL) == -1)
				err(EXIT_ERR, "sigprocmask");
			execvp(argv[1], argv + 1);
			dpy = NULL;
			err(errno == ENOENT ? EXIT_ENOENT : EXIT_EXEC, "exec");
		}
	}

	xerr = EXIT_RUNTIME;
	for (;;) {
		switch (sigtimedwait(&set, NULL, &ts)) {
		case -1:
			break;
		case SIGCHLD:
			if (wait4(child, &status, WNOHANG, NULL) == child) {
				if (WIFEXITED(status))
					return WEXITSTATUS(status);
				if (WIFSIGNALED(status))
					return EXIT_SIG + WTERMSIG(status);
			}
			break;
		default:
			return 0;
		}
		XFlush(dpy);
	}
	/* NOTREACHED */
}

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
#include <time.h> // XXX
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

static void watch(Display *dpy, XPointer client_data, int fd, Bool opening,
    XPointer *watch_data)
{
	(void)dpy;
	(void)client_data;
	(void)watch_data;
	time_t tm = time(NULL);
	fputs(ctime(&tm), stdout); // XXX
	printf("X connection: %s fd %d\n", opening ? "open" : "close", fd);
}

static void add_watch(void)
{
	int	*fdret;
	int	 i, cnt;

	if (!XInternalConnectionNumbers(dpy, &fdret, &cnt)) {
		warnx("XInternalConnectionNumbers");
		return;
	}
	printf("XInternalConnectionNumbers: cnt %d\n", cnt);
	for (i = 0; i < cnt; i++) {
		printf("fd %d\n", fdret[i]);
		if (!XAddConnectionWatch(dpy, watch, NULL))
			warnx("XAddConnectionWatch");
	}
}

int
main(int argc, char *argv[])
{
	sigset_t	set, oldset;
	pid_t		child;
	int		evbase, errbase, sig, status;

	if ((dpy = XOpenDisplay(NULL)) == NULL)
		errx(1, "no display");
	atexit(closedisplay);
	if (!XScreenSaverQueryExtension(dpy, &evbase, &errbase))
		errx(1, "X11 extension MIT-SCREEN-SAVER not supported");
	XSetErrorHandler(eh);
	XScreenSaverSuspend(dpy, True);
	add_watch();
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGCHLD);
	sigaddset(&set, SIGUSR1);
	sigprocmask(SIG_BLOCK, &set, &oldset);
	if (argc > 1) {
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
		struct timespec	ts = {
			.tv_sec = 2000,
			.tv_nsec = 0,
		};

		sig = sigtimedwait(&set, NULL, &ts);
		time_t tm = time(NULL);
		fputs(ctime(&tm), stdout); // XXX
		switch (sig) {
		case -1:
			break;
		case SIGCHLD:
			if (wait4(child, &status, WNOHANG, NULL) == child &&
			    (WIFEXITED(status) || WIFSIGNALED(status))) {
				return 0;
			}
			break;
		case SIGUSR1:
			return 127;
		default:
			printf("sig %d\n", sig); // XXX
			return 0;
		}
		XScreenSaverSuspend(dpy, True); // XXX
		int ret = XNoOp(dpy);
		printf("XNoOp %d\n", ret);
		ret = XFlush(dpy);
		printf("XFlush %d\n", ret);
		ret = XPending(dpy);
		printf("XPending %d\n", ret);
		//ret = XSync(dpy, True);
		//printf("XSync %d\n", ret);
	}
	printf("exit\n");
	if (sig == SIGUSR1)
		return 127;
	return 0;
}

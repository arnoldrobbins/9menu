/*
 * 9menu.c
 *
 * This program puts up a window that is just a menu, and executes
 * commands that correspond to the items selected.
 *
 * Initial idea: Arnold Robbins
 * Version using libXg: Matty Farrow (some ideas borrowed)
 * This code by: David Hogan and Arnold Robbins
 *
 * Arnold Robbins
 * arnold@skeeve.atl.ga.us
 * October, 1994
 */

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

char version[] = "@(#) 9menu version 1.0";

Display *dpy;		/* lovely X stuff */
int screen;
Window root;
Window menuwin;
GC gc;
unsigned long black;
unsigned long white;
XFontStruct *font;
Atom wm_protocols;
Atom wm_delete_window;
int g_argc;			/* for XSetWMProperties to use */
char **g_argv;
char *geometry = "";

char *fontlist[] = {	/* default font list if no -font */
	"pelm.latin1.9",
	"lucm.latin1.9",
	"blit",
	"9x15bold",
	"9x15",
	"lucidasanstypewriter-12",
	"fixed",
	NULL
};

#define nine_menu_width 40
#define nine_menu_height 40
static char nine_menu_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x04, 0x00,
   0x80, 0x00, 0x00, 0x04, 0x00, 0x80, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x00,
   0x00, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x04, 0x00, 0x80, 0x00, 0x00, 0x04,
   0x00, 0x80, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x00, 0x00, 0xfc, 0xff, 0xff,
   0x00, 0x00, 0x04, 0x00, 0x80, 0x00, 0x00, 0x04, 0x00, 0x80, 0x00, 0x00,
   0xfc, 0xff, 0xff, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x04, 0x00,
   0x80, 0xe0, 0x01, 0x04, 0x00, 0x80, 0xe0, 0x00, 0xfc, 0xff, 0xff, 0xe0,
   0x01, 0xfc, 0xff, 0xff, 0x20, 0x03, 0x04, 0x00, 0x80, 0x00, 0x06, 0x04,
   0x00, 0x80, 0x00, 0x0c, 0xfc, 0xff, 0xff, 0x00, 0x08, 0xfc, 0xff, 0xff,
   0x00, 0x00, 0x04, 0x00, 0x80, 0x00, 0x00, 0x04, 0x00, 0x80, 0x00, 0x00,
   0xfc, 0xff, 0xff, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x04, 0x00,
   0x80, 0x00, 0x00, 0x04, 0x00, 0x80, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x00,
   0x00, 0xfc, 0xff, 0xff, 0x00, 0x00, 0x04, 0x00, 0x80, 0x00, 0x00, 0x04,
   0x00, 0x80, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x00, 0x00, 0xfc, 0xff, 0xff,
   0x00, 0x00, 0x04, 0x00, 0x80, 0x00, 0x00, 0x04, 0x00, 0x80, 0x00, 0x00,
   0xfc, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

char *progname;		/* my name */
char *displayname;	/* X display */
char *fontname;		/* font */
char *labelname;	/* window and icon name */
int popup;		/* true if we're a popup window */
int popdown;		/* autohide after running a command */
int iconic;		/* start iconified */

char **labels;		/* list of labels and commands */
char **commands;
int numitems;

extern void usage(), run_menu(), spawn(), ask_wm_for_delete();
void set_wm_hints();

/* main --- crack arguments, set up X stuff, run the main menu loop */

int
main(argc, argv)
int argc;
char **argv;
{
	int i, j;
	char *cp;
	XGCValues gv;
	unsigned long mask;

	g_argc = argc;
	g_argv = argv;

	/* set default label name */
	if ((cp = strchr(argv[0], '/')) == NULL)
		labelname = argv[0];
	else
		labelname = ++cp;

	/* and program name for diagnostics */
	progname = labelname;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-label") == 0) {
			labelname = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-font") == 0) {
			fontname = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-display") == 0) {
			displayname = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-geometry") == 0) {
			geometry = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-popup") == 0)
			popup++;
		else if (strcmp(argv[i], "-popdown") == 0)
			popdown++;
		else if (strcmp(argv[i], "-iconic") == 0)
			iconic++;
		else if (strcmp(argv[i], "-version") == 0) {
			printf("%s\n", version);
			exit(0);
		} else if (argv[i][0] == '-')
			usage();
		else
			break;
	}

	numitems = argc - i;

	if (numitems <= 0)
		usage();

	labels = &argv[i];
	commands = (char **) calloc(numitems, sizeof(char *));
	if (commands == NULL) {
		fprintf(stderr, "%s: no memory!\n", progname);
		exit(1);
	}

	for (j = 0; j < numitems; j++) {
		if ((cp = strchr(labels[j], ':')) != NULL) {
			*cp++ = '\0';
			commands[j] = cp;
		} else
			commands[j] = labels[j];
	}

	dpy = XOpenDisplay(displayname);
	if (dpy == NULL) {
		fprintf(stderr, "%s: cannot open display", progname);
		if (displayname != NULL)
			printf(" %s", displayname);
		fprintf(stderr, "\n");
		exit(1);
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	black = BlackPixel(dpy, screen);
	white = WhitePixel(dpy, screen);

	/* try user's font first */
	if (fontname != NULL) {
		font = XLoadQueryFont(dpy, fontname);
		if (font == NULL)
			fprintf(stderr, "%s: warning: can't load font %s\n",
				progname, fontname);
	}

	/* if no user font, try one of our default fonts */
	if (font == NULL) {
		for (i = 0; fontlist[i] != NULL; i++) {
			font = XLoadQueryFont(dpy, fontlist[i]);
			if (font != NULL)
				break;
		}
	}

	if (font == NULL) {
		fprintf(stderr, "%s: fatal: cannot load a font\n", progname);
		exit(1);
	}

	gv.foreground = black^white;
	gv.background = white;
	gv.font = font->fid;
	gv.function = GXxor;
	gv.line_width = 0;
	mask = GCForeground | GCBackground | GCFunction | GCFont | GCLineWidth;
	gc = XCreateGC(dpy, root, mask, &gv);

	signal(SIGCHLD, SIG_IGN);

	run_menu();

	XCloseDisplay(dpy);
	exit(0);
}

/* spawn --- run a command */

void
spawn(com)
char *com;
{
	int pid, fd;

	pid = fork();
	if (pid < 0) {
		fprintf(stderr, "%s: can't fork\n", progname);
		return;
	} else if (pid > 0)
		return;

	close(ConnectionNumber(dpy));
	execl("/bin/sh", "sh", "-c", com, NULL);
	_exit(1);
}

/* usage --- print a usage message and die */

void
usage()
{
	fprintf(stderr, "usage: %s [-display displayname] [-font fname] ", progname);
	fprintf(stderr, "[-label name] [-popup] [-popdown] [-iconic] ");
	fprintf(stderr, "[-geometry geom] [-version] menitem:command ...\n");
	exit(1);
}

/* run_menu --- put up the window, execute selected commands */

void
run_menu()
{
	XEvent ev;
	XClientMessageEvent *cmsg;
	int i, cur, old, wide, high, status, drawn;
	int x, y, dx, dy;
	int tx, ty;

	dx = 0;
	for (i = 0; i < numitems; i++) {
		wide = strlen(labels[i]) * font->max_bounds.width + 4;
		if (wide > dx)
			dx = wide;
	}
	wide = dx;

	old = cur = -1;

	high = font->ascent + font->descent + 1;
	dy = numitems * high;

	set_wm_hints(wide, dy);

	ask_wm_for_delete();

#define	MenuMask (ButtonPressMask|ButtonReleaseMask\
	|LeaveWindowMask|PointerMotionMask|ButtonMotionMask|ExposureMask)

	XSelectInput(dpy, menuwin, MenuMask);

	XMapWindow(dpy, menuwin);

	drawn = 0;

	for (;;) {
		XNextEvent(dpy, &ev);
		switch (ev.type) {
		default:
			fprintf(stderr, "%s: unknown ev.type %d\n", 
				progname, ev.type);
			break;
		case ButtonRelease:
			if (ev.xbutton.button != Button1)
				break;
			i = ev.xbutton.y/high;
			if (ev.xbutton.x < 0 || ev.xbutton.x > wide)
				break;
			else if (i < 0 || i >= numitems)
				break;
			if (strcmp(labels[i], "exit") == 0) {
				if (commands[i] != labels[i]) {
					spawn(commands[i]);
				}
				return;
			}
			spawn(commands[i]);
			if (popup)
				return;
			if (cur >= 0 && cur < numitems)
				XFillRectangle(dpy, menuwin, gc, 0, cur*high, wide, high);
			if (popdown)
				XIconifyWindow(dpy, menuwin, screen);
			cur = -1;
			break;
		case ButtonPress:
		case MotionNotify:
			if (! drawn)
				break;
			old = cur;
			cur = ev.xbutton.y/high;
			if (ev.xbutton.x < 0 || ev.xbutton.x > wide)
				cur = -1;
			else if (cur < 0 || cur >= numitems)
				cur = -1;
			if (cur == old)
				break;
			if (old >= 0 && old < numitems)
				XFillRectangle(dpy, menuwin, gc, 0, old*high, wide, high);
			if (cur >= 0 && cur < numitems)
				XFillRectangle(dpy, menuwin, gc, 0, cur*high, wide, high);
			break;
		case LeaveNotify:
			drawn = 1;
			cur = old = -1;
			/* fall through */
		case Expose:
			if (drawn)
				XClearWindow(dpy, menuwin);
			for (i = 0; i < numitems; i++) {
				tx = (wide - strlen(labels[i])*font->max_bounds.width)/2;
				ty = i*high + font->ascent + 1;
				XDrawString(dpy, menuwin, gc, tx, ty, labels[i], strlen(labels[i]));
			}
			if (cur >= 0 && cur < numitems)
				XFillRectangle(dpy, menuwin, gc, 0, cur*high, wide, high);

			drawn = 1;
			break;
		case ClientMessage:
			cmsg = &ev.xclient;
			if (cmsg->message_type == wm_protocols
			    && cmsg->data.l[0] == wm_delete_window)
				return;
		}
	}
}

/* set_wm_hints --- set all the window manager hints */

void
set_wm_hints(wide, high)
int wide, high;
{
	Pixmap iconpixmap;
	XWMHints *wmhints;
	XSizeHints *sizehints;
	XClassHint *classhints;
	XTextProperty wname, iname;

	if ((sizehints = XAllocSizeHints()) == NULL) {
		fprintf(stderr, "%s: could not allocate size hints\n",
			progname);
		exit(1);
	}

	if ((wmhints = XAllocWMHints()) == NULL) {
		fprintf(stderr, "%s: could not allocate window manager hints\n",
			progname);
		exit(1);
	}

	if ((classhints = XAllocClassHint()) == NULL) {
		fprintf(stderr, "%s: could not allocate class hints\n",
			progname);
		exit(1);
	}

	/* fill in hints in order to parse geometry spec */
	sizehints->width = sizehints->min_width = sizehints->max_width = wide;
	sizehints->height = sizehints->min_height = sizehints->max_height = high;
	sizehints->flags = USSize|PSize|PMinSize|PMaxSize;
	if (XWMGeometry(dpy, screen, geometry, "", 1, sizehints,
			&sizehints->x, &sizehints->y,
			&sizehints->width, &sizehints->height,
			&sizehints->win_gravity) & (XValue|YValue))
		sizehints->flags |= USPosition;

	/* override -geometry for size of window */
	sizehints->width = sizehints->min_width = sizehints->max_width = wide;
	sizehints->height = sizehints->min_height = sizehints->max_height = high;

	if (XStringListToTextProperty(& labelname, 1, & wname) == 0) {
		fprintf(stderr, "%s: could allocate window name structure\n",
			progname);
		exit(1);
	}

	if (XStringListToTextProperty(& labelname, 1, & iname) == 0) {
		fprintf(stderr, "%s: could allocate icon name structure\n",
			progname);
		exit(1);
	}

	menuwin = XCreateSimpleWindow(dpy, root, sizehints->x, sizehints->y,
				sizehints->width, sizehints->height, 1, black, white);

	iconpixmap = XCreateBitmapFromData(dpy, menuwin,
					   nine_menu_bits,
					   nine_menu_width,
					   nine_menu_height);

	wmhints->icon_pixmap = iconpixmap;
	wmhints->input = False;		/* no keyboard input */
	if (iconic)
		wmhints->initial_state = IconicState;
	else
		wmhints->initial_state = NormalState;

	wmhints->flags = IconPixmapHint | StateHint | InputHint;

	classhints->res_name = progname;
	classhints->res_class = "9menu";

	XSetWMProperties(dpy, menuwin, & wname, & iname,
		g_argv, g_argc, sizehints, wmhints, classhints);
}

/* ask_wm_for_delete --- jump through hoops to ask window manager to delete us */

void
ask_wm_for_delete()
{
	int status;

	wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wm_delete_window = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	status = XSetWMProtocols(dpy, menuwin, & wm_delete_window, 1);

	if (status != True)
		fprintf(stderr, "%s: could not ask for clean delete\n",
			progname);
}

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
 * Copyright (c), Arnold Robbins and David Hogan
 *
 * Arnold Robbins
 * arnold@skeeve.com
 * October, 1994
 *
 * Code added to cause pop-up (unIconify) to move menu to mouse.
 * Christopher Platt
 * platt@coos.dartmouth.edu
 * May, 1995
 *
 * Said code moved to -teleport option, and -warp option added.
 * Arnold Robbins
 * June, 1995
 *
 * Code added to allow -fg and -bg colors.
 * John M. O'Donnell
 * odonnell@stpaul.lampf.lanl.gov
 * April, 1997
 *
 * Code added for -file and -path options.
 * Peter Seebach
 * seebs@plethora.net
 * October, 2001
 *
 * Code added to allow up and down arrow keys to go up
 * and down menu and RETURN to select an item.
 * Matthias Bauer
 * bauerm@immd1.informatik.uni-erlangen.de
 * June, 2003
 *
 * spawn() changed to do exec directly if -popup, based on
 * suggestion from
 * Andrew Stribblehill
 * a.d.stribblehill@durham.ac.uk
 * June, 2004
 *
 * Allow "-" to mean read info from stdin.
 * suggestion from
 * Peter Bailey, by way of
 * Andrew Stribblehill
 * a.d.stribblehill@durham.ac.uk
 * August, 2005
 *
 * Fix compile warnings (getcwd and getting a key sym).
 * Arnold Robbins
 * January, 2015.
 */

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>

char version[] = "@(#) 9menu version 1.9";

Display *dpy;		/* lovely X stuff */
int screen;
Window root;
Window menuwin;
GC gc;
unsigned long black;
unsigned long white;
char *fgcname = NULL;
char *bgcname = NULL;
Colormap defcmap;
XColor color;
XFontStruct *font;
Atom wm_protocols;
Atom wm_delete_window;
int g_argc;			/* for XSetWMProperties to use */
char **g_argv;
int f_argc;			/* for labels read from files */
char **f_argv;
char *geometry = "";
int savex, savey;
Window savewindow;

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

/* the 9menu icon, for garish window managers */
#define nine_menu_width 40
#define nine_menu_height 40
static unsigned char nine_menu_bits[] = {
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

/* Modify this to your liking */
#define CONFIG_MENU_UP_KEY  XK_Up
#define CONFIG_MENU_DOWN_KEY    XK_Down
#define CONFIG_MENU_SELECT_KEY  XK_Return

char *progname;		/* my name */
char *displayname;	/* X display */
char *fontname;		/* font */
char *labelname;	/* window and icon name */
char *filename;		/* file to read options or labels from */
int popup;		/* true if we're a popup window */
int popdown;		/* autohide after running a command */
int iconic;		/* start iconified */
int teleport;		/* teleport the menu */
int warp;		/* warp the mouse */

char **labels;		/* list of labels and commands */
char **commands;
int numitems;

char *shell = "/bin/sh";	/* default shell */

extern void usage(), run_menu(), spawn(), ask_wm_for_delete();
extern void reap(), set_wm_hints();
extern void redraw(), teleportmenu(), warpmouse(), restoremouse();
extern void memory();
extern int args();

/* memory --- print the out of memory message and die */

void
memory(char *s)
{
	fprintf(stderr, "%s: couldn't allocate memory for %s\n", progname, s);
	exit(1);
}

/* args --- go through the argument list, set options */

int
args(int argc, char **argv)
{
	int i;

	if (argc == 0 || argv == NULL || argv[0] == '\0')
		return -1;

	for (i = 0; i < argc && argv[i] != NULL; i++) {
		if (strcmp(argv[i], "-display") == 0) {
			displayname = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-file") == 0) {
			filename = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-font") == 0) {
			fontname = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-geometry") == 0) {
			geometry = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-label") == 0) {
			labelname = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-shell") == 0) {
			shell = argv[i+1];
			i++;
		} else if (strcmp(argv[i], "-popup") == 0)
			popup++;
		else if (strcmp(argv[i], "-popdown") == 0)
			popdown++;
		else if (strcmp(argv[i], "-fg") == 0)
			fgcname = argv[++i];
		else if (strcmp(argv[i], "-bg") == 0)
			bgcname = argv[++i];
		else if (strcmp(argv[i], "-iconic") == 0)
			iconic++;
		else if (strcmp(argv[i], "-path") == 0) {
			char pathbuf[MAXPATHLEN];
			char *s, *t;

			s = getenv("PATH");
			if (s != NULL) {
				/* append current dir to PATH */
				char *cp = getcwd(pathbuf, MAXPATHLEN);
				t = malloc(strlen(s) + strlen(pathbuf) + 7);
				sprintf(t, "PATH=%s:%s", pathbuf, s);
				putenv(t);
			}
		} else if (strcmp(argv[i], "-teleport") == 0)
			teleport++;
		else if (strcmp(argv[i], "-warp") == 0)
			warp++;
		else if (strcmp(argv[i], "-version") == 0) {
			printf("%s\n", version);
			exit(0);
		} else if (argv[i][0] == '-')
			usage();
		else
			break;
	}
	return i;
}

/* main --- crack arguments, set up X stuff, run the main menu loop */

int
main(int argc, char **argv)
{
	int i, j;
	char *cp;
	XGCValues gv;
	unsigned long mask;
	int nlabels = 0;

	g_argc = argc;
	g_argv = argv;

	/* set default label name */
	if ((cp = strrchr(argv[0], '/')) == NULL)
		labelname = argv[0];
	else
		labelname = ++cp;

	++argv;
	--argc;

	/* and program name for diagnostics */
	progname = labelname;

	i = args(argc, argv);

	numitems = argc - i;

	if (numitems <= 0 && filename == NULL)
		usage();

	if (filename) {
		/* Read options and labels from file */
		char fbuf[1024];
		FILE *fp;

		if (strcmp(filename, "-") == 0) {
			fp = stdin;
		} else {
			fp = fopen(filename, "r");
		}
		if (fp == NULL) {
			fprintf(stderr, "%s: couldn't open '%s'\n", progname,
				filename);
			exit(1);
		}
		while (fgets(fbuf, sizeof fbuf, fp)) {
			char *s = fbuf;
			strtok(s, "\n");
			if (s[0] == '-') {
				char *temp[3];
				temp[0] = s;
				temp[1] = strchr(s, ' ');
				if (temp[1]) {
					*(temp[1]++) = '\0';
					s = malloc(strlen(temp[1]) + 1);
					if (s == NULL)
						memory("temporary argument");
					strcpy(s, temp[1]);
					temp[1] = s;
				}
				temp[2] = 0;
				args(temp[1] ? 2 : 1, temp);
				continue;
			}
			if (s[0] == '#')
				continue;
			/* allow - in menu items to be escaped */
			if (s[0] == '\\')
				++s;
			/* allocate space */
			if (f_argc < nlabels + 1) {
				int k;
				char **temp = malloc(sizeof(char *) * (f_argc + 5));
				if (temp == 0)
					memory("temporary item");

				for (k = 0; k < nlabels; k++)
					temp[k] = f_argv[k];

				free(f_argv);
				f_argv = temp;
				f_argc += 5;
			}
			f_argv[nlabels] = malloc(strlen(s) + 1);
			if (f_argv[nlabels] == NULL)
				memory("temporary text");
			strcpy(f_argv[nlabels], s);
			++nlabels;
		}
	}

	labels = (char **) malloc((numitems + nlabels) * sizeof(char *));
	commands = (char **) malloc((numitems + nlabels) * sizeof(char *));
	if (commands == NULL || labels == NULL)
		memory("command and label arrays");

	for (j = 0; j < numitems; j++) {
		labels[j] = argv[i + j];
		if ((cp = strchr(labels[j], ':')) != NULL) {
			*cp++ = '\0';
			commands[j] = cp;
		} else
			commands[j] = labels[j];
	}

	/*
	 * Now we no longer need i (our offset into argv) so we recycle it,
	 * while keeping the old value of j!
	 */
	for (i = 0; i < nlabels; i++) {
		labels[j] = f_argv[i];
		if ((cp = strchr(labels[j], ':')) != NULL) {
			*cp++ = '\0';
			commands[j] = cp;
		} else
			commands[j] = labels[j];
		++j;
	}

	/* And now we merge the totals */
	numitems += nlabels;

	dpy = XOpenDisplay(displayname);
	if (dpy == NULL) {
		fprintf(stderr, "%s: cannot open display", progname);
		if (displayname != NULL)
			fprintf(stderr, " %s", displayname);
		fprintf(stderr, "\n");
		exit(1);
	}
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	/*
	 * This used to be
	 * black = BlackPixel(dpy, screen);
	 * white = WhitePixel(dpy, screen);
	 */
	defcmap = DefaultColormap(dpy, screen);
	if (fgcname == NULL
	    || XParseColor(dpy, defcmap, fgcname, &color) == 0
	    || XAllocColor(dpy, defcmap, &color) == 0)
		black = BlackPixel(dpy, screen);
	else
		black = color.pixel;

	if (bgcname == NULL
	    || XParseColor(dpy, defcmap, bgcname, &color) == 0
	    || XAllocColor(dpy, defcmap, &color) == 0)
		white = WhitePixel(dpy, screen);
	else
		white = color.pixel;

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

	signal(SIGCHLD, reap);

	run_menu();

	XCloseDisplay(dpy);
	exit(0);
}

/* spawn --- run a command */

void
spawn(char *com)
{
	int pid;
	static char *sh_base = NULL;

	if (sh_base == NULL) {
		sh_base = strrchr(shell, '/');
		if (sh_base != NULL)
			sh_base++;
		else
			sh_base = shell;
	}

	/*
	 * Since -popup means run command and exit, just
	 * fall straight into exec code.  Thus only fork
	 * if not popup.
	 */
	if (! popup) {
		if (strncmp(com, "exec ", 5) != 0) {
			pid = fork();
			if (pid < 0) {
				fprintf(stderr, "%s: can't fork\n", progname);
				return;
			} else if (pid > 0)
				return;
		} else {
			com += 5;
		}
	}

	close(ConnectionNumber(dpy));
	execl(shell, sh_base, "-c", com, NULL);
	execl("/bin/sh", "sh", "-c", com, NULL);
	_exit(1);
}

/* reap --- collect dead children */

void
reap(int s)
{
	(void) wait((int *) NULL);
	signal(s, reap);
}

/* usage --- print a usage message and die */

void
usage()
{
	fprintf(stderr, "usage: %s [-display displayname] [-font fname] ", progname);
	fprintf(stderr, "[-file filename] [-path]");
	fprintf(stderr, "[-geometry geom] [-shell shell]  [-label name] ");
	fprintf(stderr, "[-popup] [-popdown] [-iconic]  [-teleport] ");
	fprintf(stderr, "[-warp]  [-version] menitem:command ...\n");
	exit(0);
}

/* run_menu --- put up the window, execute selected commands */

void
run_menu()
{
	XEvent ev;
	XClientMessageEvent *cmsg;
	KeySym key;
	int i, cur, old, wide, high, ico, dx, dy;

	dx = 0;
	for (i = 0; i < numitems; i++) {
		wide = XTextWidth(font, labels[i], strlen(labels[i])) + 4;
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
	|LeaveWindowMask|PointerMotionMask|ButtonMotionMask\
	|ExposureMask|StructureNotifyMask|KeyPressMask)

	XSelectInput(dpy, menuwin, MenuMask);

	XMapWindow(dpy, menuwin);

	ico = 1;	/* warp to first item */
	i = 0;		/* save menu Item position */

	for (;;) {
		XNextEvent(dpy, &ev);
		switch (ev.type) {
		default:
			fprintf(stderr, "%s: unknown ev.type %d\n",
				progname, ev.type);
			break;
		case ButtonRelease:
			/* allow button 1 or button 3 */
			if (ev.xbutton.button == Button2)
				break;
			i = ev.xbutton.y/high;
			if (ev.xbutton.x < 0 || ev.xbutton.x > wide)
				break;
			else if (i < 0 || i >= numitems)
				break;
			if (warp)
				restoremouse();
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
		case KeyPress:
			/* http://stackoverflow.com/questions/9838385/replace-of-xkeycodetokeysym */
			key = XkbKeycodeToKeysym(dpy, ev.xkey.keycode, 0,
					ev.xkey.state & ShiftMask ? 1 : 0);	
			if (key != CONFIG_MENU_UP_KEY
			    && key != CONFIG_MENU_DOWN_KEY
			    && key != CONFIG_MENU_SELECT_KEY)
				break;

			if (key == CONFIG_MENU_UP_KEY) {
				old = cur;
				cur--;
			} else if (key == CONFIG_MENU_DOWN_KEY) {
				old = cur;
				cur++;
			}
			
			while (cur < 0)
				cur += numitems;
		
			cur %= numitems;

			if (key == CONFIG_MENU_UP_KEY || key == CONFIG_MENU_DOWN_KEY) {
				if (cur == old)
					break;
				if (old >= 0 && old < numitems && cur != -1)
					XFillRectangle(dpy, menuwin, gc, 0, old*high, wide, high);
				if (cur >= 0 && cur < numitems && cur != -1)
					XFillRectangle(dpy, menuwin, gc, 0, cur*high, wide, high);
				break;
			}

			if (warp)
				restoremouse();
			if (key == CONFIG_MENU_SELECT_KEY) {
				if (strcmp(labels[cur], "exit") == 0) {
					if (commands[cur] != labels[cur]) {
						spawn(commands[cur]);
					}
					return;
				}
				spawn(commands[cur]);
			}

			if (popup)
				return;
			if (popdown)
				XIconifyWindow(dpy, menuwin, screen);
			break;
		case LeaveNotify:
			cur = old = -1;
			XClearWindow(dpy, menuwin);
			redraw(cur, high, wide);
			break;
		case ReparentNotify:
		case ConfigureNotify:
			/*
			 * ignore these, they come from XMoveWindow
			 * and are enabled by Struct..
			 */
			break;
		case UnmapNotify:
			ico = 1;
			XClearWindow(dpy, menuwin);
			break;
		case MapNotify:
			if (ico) {
				if (teleport)
					teleportmenu(i, wide, high);
				else if (warp)
					warpmouse(i, wide, high);
			}
			XClearWindow(dpy, menuwin);
			redraw(cur = i, high, wide);
			ico = 0;
			break;
		case Expose:
			XClearWindow(dpy, menuwin);
			redraw(cur, high, wide);
			break;
		case ClientMessage:
			cmsg = &ev.xclient;
			if (cmsg->message_type == wm_protocols
			    && cmsg->data.l[0] == wm_delete_window)
				return;
		case MappingNotify:	/* why do we get this? */
			break;
		}
	}
}

/* set_wm_hints --- set all the window manager hints */

void
set_wm_hints(int wide, int high)
{
	Pixmap iconpixmap;
	XWMHints *wmhints;
	XSizeHints *sizehints;
	XClassHint *classhints;
	XTextProperty wname, iname;

	if ((sizehints = XAllocSizeHints()) == NULL)
		memory("size hints");

	if ((wmhints = XAllocWMHints()) == NULL)
		memory("window manager hints");

	if ((classhints = XAllocClassHint()) == NULL)
		memory("class hints");

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

	if (XStringListToTextProperty(& labelname, 1, & wname) == 0)
		memory("window name structure");

	if (XStringListToTextProperty(& labelname, 1, & iname) == 0)
		memory("icon name structure");

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

#ifdef SET_PROPERTIES_MANUALLY
	/*
	 * For some reason, XSetWMProperties (see below) is failing
	 * John O'Donnell replaces it with the following commands
	 * (this leaves out XSetWMClientMachine,
	 * and also environment variable checking from ClassHint)
	 */
	XSetWMName(dpy, menuwin, &wname);
	XSetWMIconName(dpy, menuwin, &iname);
	XSetCommand(dpy, menuwin, g_argv, g_argc);
	XSetWMHints(dpy, menuwin, wmhints);
	XSetClassHint(dpy, menuwin, classhints);
	XSetWMNormalHints(dpy, menuwin, sizehints);
#else
	XSetWMProperties(dpy, menuwin, & wname, & iname,
		g_argv, g_argc, sizehints, wmhints, classhints);
#endif
}

/* ask_wm_for_delete --- jump through hoops to ask WM to delete us */

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

/* redraw --- actually redraw the menu */

void
redraw(int cur, int high, int wide)
{
	int tx, ty, i;

	for (i = 0; i < numitems; i++) {
		tx = (wide - XTextWidth(font, labels[i], strlen(labels[i]))) / 2;
		ty = i*high + font->ascent + 1;
		XDrawString(dpy, menuwin, gc, tx, ty, labels[i], strlen(labels[i]));
	}
	if (cur >= 0 && cur < numitems)
		XFillRectangle(dpy, menuwin, gc, 0, cur*high, wide, high);
}

/* teleportmenu --- move the menu to the right place */

void
teleportmenu(int cur, int wide, int high)
{
	int x, y, dummy;
	Window wdummy;

	if (XQueryPointer(dpy, menuwin, &wdummy, &wdummy, &x, &y,
			       &dummy, &dummy, &dummy))
		XMoveWindow(dpy, menuwin, x-wide/2, y-cur*high-high/2);
}

/* warpmouse --- bring the mouse to the menu */

void
warpmouse(int cur, int wide, int high)
{
	int dummy;
	Window wdummy;
	int offset;

	/* move tip of pointer into middle of menu item */
	offset = (font->ascent + font->descent + 1) / 2;
	offset += 6;	/* fudge factor */

	if (XQueryPointer(dpy, menuwin, &wdummy, &wdummy, &savex, &savey,
			       &dummy, &dummy, &dummy))
		XWarpPointer(dpy, None, menuwin, 0, 0, 0, 0,
				wide/2, cur*high-high/2+offset);
}

/* restoremouse --- put the mouse back where it was */

void
restoremouse()
{
	XWarpPointer(dpy, menuwin, root, 0, 0, 0, 0,
				savex, savey);
}

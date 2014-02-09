/** \file
 * Map /dev/input/event* to XTestEvents.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <X11/Xproto.h>
#include <X11/extensions/XTest.h>
#include <X11/Xlib.h>
#include <unistd.h>
#include <err.h>

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>
// #include "util.h"


static Display * dpy;

// table from http://lists.freedesktop.org/pipermail/xorg/2006-May/015587.html
// arrow keys are borked; need to revisit this.
static const int keymap[255] = {
	// this doesn't work.  bummer;
	[0] = XK_Z,
	[1] = XK_S,
	[2] = XK_W,
	[3] = XK_3,
	[4] = XK_X,
	[5] = XK_D,
	[6] = XK_E,
	[7] = XK_4,
	[8] = XK_C,
	[9] = XK_F,

	[10] = XK_R,
	[11] = XK_5,
	[12] = XK_V,
	[13] = XK_G,
	[14] = XK_T,
	[15] = XK_6,
	[16] = XK_B,
	[17] = XK_H,
	[18] = XK_Y,
	[19] = XK_7,

	[20] = XK_N,
	[21] = XK_J,
	[22] = XK_U,
	[23] = XK_B,
	[24] = XK_KP_2,
	[25] = XK_KP_5,
	[26] = XK_KP_8,
	[27] = XK_Num_Lock,
	[28] = XK_M,
	[29] = XK_K,

	[30] = XK_I,
	[31] = XK_9,
	[32] = XK_comma,
	[33] = XK_L,
	[34] = XK_O,
	[35] = XK_0,
	[36] = XK_period,
	[37] = XK_semicolon,
	[38] = XK_p,
	[39] = XK_minus,

	[40] = XK_slash,
	[41] = XK_apostrophe,
	[42] = XK_braceleft,
	[43] = XK_equal,
	[44] = XK_space,
	[45] = XK_Shift_R,
	[46] = XK_backslash,
	[47] = XK_braceright,
	[48] = XK_Caps_Lock,
	[49] = XK_KP_Multiply,

	[50] = XK_Return,
	[51] = XK_BackSpace,
	[52] = XK_KP_0,
	[53] = XK_KP_1,
	[54] = XK_KP_4,
	[55] = XK_KP_7,
	[56] = XK_KP_Add,
	[57] = 0,
	[58] = XK_KP_Subtract,
	[59] = XK_Scroll_Lock,

	[60] = XK_KP_Decimal,
	[61] = XK_KP_3,
	[62] = XK_KP_6,
	[63] = XK_KP_9,
	[64] = 0,
	[65] = XK_A,
	[66] = XK_Q,
	[67] = XK_2,
	[68] = XK_Alt_L,
	[69] = 0,

	[70] = 0,
	[71] = XK_1,
	[72] = XK_F7,
	[73] = XK_F5,
	[74] = XK_F3,
	[75] = XK_F1,
	[76] = XK_F8,
	[77] = XK_F6,
	[78] = XK_F4,
	[79] = XK_F2,

	[80] = XK_F10,
	[81] = 0,
	[82] = 0,
	[83] = 0,
	[84] = XK_F9,
	[85] = 0,
	[86] = 0,
	[87] = 0,
	[88] = XK_Shift_L,
	[89] = XK_Control_L,

	[90] = XK_Tab,
	[91] = XK_Escape,
	[92] = 0,
	[93] = 0,
	[94] = 0,
	[95] = 0,
};

void FakeKeyEvent(Display* dpy,unsigned int key, Bool is_press, unsigned long delay) {
   Window root = XDefaultRootWindow(dpy);
   Window focus = 0;
   int revert;
   //XGetInputFocus(dpy, &focus, &revert);
   XKeyEvent event;
   event.display = dpy;
   event.window = focus;
   event.root = root;
   event.subwindow = None;
   event.time = CurrentTime;
   event.x = event.y = event.x_root = event.y_root = 1;
   event.same_screen = True;
   event.keycode = XKeysymToKeycode(dpy, key);
   event.state = 0;
   event.type = is_press?KeyPress:KeyRelease;
   XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);
   printf("event sent!\n");
}

#define die warn

typedef struct {
  __u8 code;
  __u8 up;
} SimpleEvt;

static void
read_one(
	int fd
)
{
  //printf("SELECTED\n");
	const int max_evs = 8;
	SimpleEvt evs[max_evs];
	const ssize_t rlen = read(fd, evs, sizeof(evs));
	if (rlen < 0)
		die("read failed");

	const int num_ev = rlen / sizeof(*evs);

	int mouse_valid = 0;
	int mx = 0;
	int my = 0;

	for (int i = 0 ; i < num_ev ; i++)
	{
		const SimpleEvt * const ev = &evs[i];
		//if (0)
                //printf("code=%x up=%d\n",
                //       ev->code,
                //       ev->up
                //       );

                int button = ev->code;
                int is_press = ev->up;
                int key = keymap[button];
                if (key != 0)
		{
                  KeyCode code = XKeysymToKeycode(display, key);
                  XTestFakeKeyEvent(dpy, code, is_press, 0);
                   	//FakeKeyEvent(dpy, key, is_press, 0);
		}
                else
                  printf("EV_KEY code=%d->%d unhandled\n", ev->code, key);
        }
	XFlush(dpy);
}


int
main(
	int argc,
	char ** argv
)
{
	dpy = XOpenDisplay(NULL);
	if (!dpy)
		die("Unable to open display\n");
	
	const int num_fds = argc - 1;
	int * fds = calloc(sizeof(*fds), num_fds);
	int max_fd = 0;

	for (int i = 0 ; i < num_fds ; i++)
	{
		const char * const devname = argv[i+1];
		const int fd = open(devname, O_RDONLY, 0666);
		if (fd < 0)
			die("%s: failed to open\n", devname);
		fds[i] = fd;

		printf("%s (fd %d)\n", devname, fd);
		if (fd > max_fd)
			max_fd = fd;
	}

	while (1)
	{
		fd_set read_fds;
		FD_ZERO(&read_fds);
		for (int i = 0 ; i < num_fds ; i++)
		{
			const int fd = fds[i];
			FD_SET(fd, &read_fds);
		}

		int rc = select(max_fd+1, &read_fds, NULL, NULL, NULL);
		if (rc < 0)
			die("select\n");

		for (int i = 0 ; i < num_fds ; i++)
		{
			const int fd = fds[i];
			if (FD_ISSET(fd, &read_fds))
				read_one(fd);
		}
	}
}

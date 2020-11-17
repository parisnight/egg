/* Interface for X-windows Version 11       3/28/94 */
/* gcc -shared -o xwin.o xwin.c thirty year old hack survives */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

Display *d;
Window rw, w;
GC gc;
XSetWindowAttributes at;
unsigned long col[16];

void setcolor(d)
Display *d;
{
  static char colorname[16][15] = {
    "light gray", "slate blue", "red", "yellow",
    "green", "magenta", "cyan", "black",
    "white", "lime green", "coral", "khaki",
    "LightSlateGray", "orange", "blue", "blue violet"
  };
  Colormap colmap;
  XColor c0, c1;
  int i;

  colmap = XDefaultColormap(d, 0);
  for (i = 0; i < 16; i++) {
    XAllocNamedColor(d, colmap, colorname[i], &c1, &c0);
    col[i] = c1.pixel;
  }
}

void xopen(int x0, int y0, int x1, int y1)
{
  if (i == 3) {
    gotoy;
  }
  Font font;
/* XSetWindowAttributes at; */

  d = XOpenDisplay(NULL);
  setcolor(d);

  rw = XDefaultRootWindow(d);
  w = XCreateSimpleWindow(d, rw, x0, y0, x1, y1, 1, col[14], col[0]);

  at.override_redirect = 1;
  at.event_mask = ButtonPressMask;
  XChangeWindowAttributes(d, w, CWOverrideRedirect | CWEventMask, &at);
/* at.bit_gravity= 65535; 
XChangeWindowAttributes(d,w,CWBitGravity,&at); */

  XMapRaised(d, w);
  gc = XCreateGC(d, rw, 0, 0);
/* XSetFunction(d,gc,GXset); */

  font = XLoadFont(d,
/* 98.2 "-adobe-helvetica-bold-o-normal--17-120-100-100-p-92-iso8859-1");*/
                   "-*-helvetica-*-*-*-*-*-100-100-100-*-*-*-*");
  XSetFont(d, gc, font);
  XSetForeground(d, gc, col[2]);
  XSetLineAttributes(d, gc, 2, LineSolid, CapRound, JoinRound);
}

void xsetline(int thickness, int c)
{
  XSetLineAttributes(d, gc, thickness, LineSolid, CapRound, JoinRound);
  XSetForeground(d, gc, col[c]);
}

void xline(x, y, x1, y1)
int x, y, x1, y1;
{
/*  XSetPlaneMask(d,gc,AllPlanes); 
XSetForeground(d,gc,col[c]);*/
  XDrawLine(d, w, gc, x, y, x1, y1);
/* XFlush(d);  MUST FLUSH, otherwise lose some lines that are queued up */
}                               /* flush moved to xclose() */

void xlines(short *vertices, int npts)
{
  XDrawLines(d, w, gc, (XPoint *) vertices, npts, CoordModeOrigin);
}

void xprint(x, y, str, c)
int x, y, c;
char *str;
{
  XSetForeground(d, gc, col[c]);
  XDrawString(d, w, gc, x, y, str, strlen(str));
}

/* before 98.2  This one worked only on Solaris, failed on BSD and Linux
fpoint ginxwin()
{
static fpoint f;
XButtonPressedEvent bp;

XFlush(d);
while (! XEventsQueued(d,QueuedAfterFlush));
XNextEvent(d,(XEvent*) &bp);
f.x=bp.x; f.y= posy - bp.y;
printf("%f %f\n",f.x,f.y);
return(f);
} */

XButtonEvent *ginxwin()
{                               /* 98.2 */
/*static float f[2];*/
  static XEvent event;

/*XFlush(d);
while (! XEventsQueued(d,QueuedAfterFlush));
*/
  do
    XNextEvent(d, &event);
  while (event.type != ButtonPress);
/*f[0]= event.xbutton.x; f[1]= event.xbutton.y;
printf("%f %f\n",f.x,f.y);*/
  return (&event.xbutton);
}

void xflush()
{
  XFlush(d);
}

void xclose()
{
  XFlush(d);
/* getchar();
XFreeGC(d,gc);
XDestroyWindow(d,w);
XFlush(d);
XCloseDisplay(d); */
}

void main()
{
  short p[] = { 4, 4, 4, 40, 40, 40, 40, 4, 4, 4 };
  xopen(100, 100, 600, 560);
  xsetline(3, 10);
  xline(0, 0, 100, 100);
  xlines(p, 5);
  xprint(10, 50, "hello world", 4);
  xflush();                     /* necessary to see any window at all */
  printf("ginxwin %d\n", ginxwin()->x);
  sleep(10);
}

/* miscellaneous x window notes
at.backing_store=true; CWBackingStore;
XSetFillRule(d,gc,rule);   use the Winding rule to fill in everything
XSetFunction uses such integers as GXor GXand GXxor,  see X.h for explanation
#include <X11/cursorfont.h> XRecolorCursor()
XDefineCursor()
Programmers Guide: XWIN   Xlib-C Language Interface   Graphics Series   at UMlib
*/

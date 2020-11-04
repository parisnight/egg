/* Interface for X-windows Version 11       3/28/94  roa */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "n.h"
#define posy 560

Display *d;
Window rw,w;
GC gc;
XSetWindowAttributes at;
unsigned long col[16];

setcolor (d,c)
Display *d;
unsigned long c[];
{ 
static char colorname[16][15]={
"light gray","slate blue","red","yellow",
"green","magenta","cyan","black",
"white","lime green","coral","khaki",
"LightSlateGray","orange","blue","blue violet"};
Colormap colmap;
XColor c0,c1;
int i;

colmap=XDefaultColormap(d,0);
for (i=0;i<16; i++) {
XAllocNamedColor(d,colmap,colorname[i],&c1,&c0);
c[i]=c1.pixel;
}
}
 
xopen(tr)
struct transf *tr;
{
Font font;
/* XSetWindowAttributes at; */

d=XOpenDisplay(NULL);
setcolor(d,&col[0]);

rw=XDefaultRootWindow(d);
/* w=XCreateSimpleWindow(d,rw,400,200,posy,posy,5,col[14],col[0]); 
*/
w=XCreateSimpleWindow(d,rw,200,200,posy,posy,1,col[14],col[0]); 

at.override_redirect=1;
at.event_mask=ButtonPressMask;
XChangeWindowAttributes(d,w,CWOverrideRedirect | CWEventMask,&at);
/* at.bit_gravity= 65535; 
XChangeWindowAttributes(d,w,CWBitGravity,&at); */

XMapRaised (d,w);
gc=XCreateGC(d,rw,0,0);
/* XSetFunction(d,gc,GXset); */

font=XLoadFont(d,
/* 98.2 roa "-adobe-helvetica-bold-o-normal--17-120-100-100-p-92-iso8859-1");*/
 "-*-helvetica-*-*-*-*-*-100-100-100-*-*-*-*");
XSetFont(d,gc,font);
XSetForeground(d,gc,col[2]);
XSetLineAttributes(d,gc, 2,LineSolid,CapRound,JoinRound);
}

xlin(x,y,x1,y1,c)
int x,y,x1,y1; int c;
{
XSetForeground(d,gc,col[c]);
/*  XSetPlaneMask(d,gc,AllPlanes); */
XDrawLine(d,w,gc,x,posy-y,x1,posy-y1);
/* XFlush(d);  MUST FLUSH, otherwise lose some lines that are queued up */
}             /* flush moved to xclose() */

xprint(x,y,str,c)
int x,y,c;
char *str;
{
XSetForeground(d,gc,col[c]);
XDrawString(d,w,gc,x,posy-y,str,strlen(str));
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

fpoint ginxwin() /* 98.2 roa */
{
static fpoint f;
XEvent event;

/*XFlush(d);
while (! XEventsQueued(d,QueuedAfterFlush));
*/
do XNextEvent(d, &event);
while (event.type != ButtonPress);
f.x= event.xbutton.x; f.y= posy - event.xbutton.y;
/*printf("%f %f\n",f.x,f.y);*/
return(f);
}


xclose ()
{
XFlush(d);
/* getchar();
XFreeGC(d,gc);
XDestroyWindow(d,w);
XFlush(d);
XCloseDisplay(d); */
}

/* miscellaneous x window notes
at.backing_store=true; CWBackingStore;
XSetFillRule(d,gc,rule);   use the Winding rule to fill in everything
XSetFunction uses such integers as GXor GXand GXxor,  see X.h for explanation
#include <X11/cursorfont.h> XRecolorCursor()
XDefineCursor()
Programmers Guide: XWIN   Xlib-C Language Interface   Graphics Series   at UMlib
*/

/* fbgraph.c  2020.12.10 */
/* gcc -shared -o fbgraph.so fbgraph.c */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

struct fb_var_screeninfo vinfo, orig_vinfo;
struct fb_fix_screeninfo finfo;
int fbfd = 0;
char *fbp = 0;
int color;

void putpixel(int x, int y)
{
  *(int *) (fbp + 4 * x + y * finfo.line_length) = color;
}

/* Bresenham's line algorithm */
void draw_line(int x0, int y0, int x1, int y1)
{
  int sx, sy, err, e2, done = 0;
  int dx = x1 - x0;
  int dy = y1 - y0;
  dx = (dx >= 0) ? dx : -dx;
  dy = (dy >= 0) ? dy : -dy;
  if (x0 < x1)
    sx = 1;
  else
    sx = -1;
  if (y0 < y1)
    sy = 1;
  else
    sy = -1;
  err = dx - dy;
  while (!done) {
    putpixel(x0, y0);
    if ((x0 == x1) && (y0 == y1))
      done = 1;
    else {
      e2 = 2 * err;
      if (e2 > -dy) {
        err = err - dy;
        x0 = x0 + sx;
      }
      if (e2 < dx) {
        err = err + dx;
        y0 = y0 + sy;
      }
    }
  }
}

/* rectangle */
void draw_rect(int x0, int y0, int w, int h)
{
  draw_line(x0, y0, x0 + w, y0);
  draw_line(x0, y0, x0, y0 + h);
  draw_line(x0, y0 + h, x0 + w, y0 + h);
  draw_line(x0 + w, y0, x0 + w, y0 + h);
}

/* filled rectangle */
void fill_rect(int x0, int y0, int w, int h)
{
  int y;
  for (y = 0; y < h; y++) {
    draw_line(x0, y0 + y, x0 + w, y0 + y);
  }
}

/* Bresenham's circle algorithm */
void draw_circle(int x0, int y0, int r)
{
  int x = r, y = 0;
  int radiusError = 1 - x;

  while (x >= y) {
    /* top l, r, upper mid l, r, lower mid l, r, bottom l, r */
    putpixel(-y + x0, -x + y0);
    putpixel(y + x0, -x + y0);
    putpixel(-x + x0, -y + y0);
    putpixel(x + x0, -y + y0);
    putpixel(-x + x0, y + y0);
    putpixel(x + x0, y + y0);
    putpixel(-y + x0, x + y0);
    putpixel(y + x0, x + y0);
    y++;
    if (radiusError < 0) {
      radiusError += 2 * y + 1;
    } else {
      x--;
      radiusError += 2 * (y - x + 1);
    }
  }
}

/* filled circle */
void fill_circle(int x0, int y0, int r)
{
  int x = r, y = 0;
  int radiusError = 1 - x;

  while (x >= y) {
    /* top, upper mid, lower mid, bottom */
    draw_line(-y + x0, -x + y0, y + x0, -x + y0);
    draw_line(-x + x0, -y + y0, x + x0, -y + y0);
    draw_line(-x + x0, y + y0, x + x0, y + y0);
    draw_line(-y + x0, x + y0, y + x0, x + y0);
    y++;
    if (radiusError < 0) {
      radiusError += 2 * y + 1;
    } else {
      x--;
      radiusError += 2 * (y - x + 1);
    }
  }
}

void fblines(short *p, int npts)
{
  int i;
  for (i = 0; i < npts - 1; i++, p+=2)
    draw_line(p[0],p[1],p[2],p[3]);
}

void drawgrid()
{
  int x, y;
  for (y = 0; y < (vinfo.yres); y += 10) {
    for (x = 0; x < vinfo.xres; x++) {
      putpixel(x, y);
    }
  }
  for (x = 0; x < vinfo.xres; x += 10) {
    for (y = 0; y < (vinfo.yres); y++) {
      putpixel(x, y);
    }
  }
}

void fbopen()
{
  fbfd = open("/dev/fb0", O_RDWR);
  if (fbfd == -1) {
    printf("Error: cannot open framebuffer device.\n");
  }

  if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
    printf("Error reading variable information.\n");
  }
  printf("vscreeninfo %dx%d, %dbpp\n", vinfo.xres, vinfo.yres,
         vinfo.bits_per_pixel);

  /* save for restoring (copy vinfo to vinfo_orig) */
  memcpy(&orig_vinfo, &vinfo, sizeof(struct fb_var_screeninfo));

  /*vinfo.bits_per_pixel = 8; force 8 bit */
  if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo)) {
    printf("Error setting variable information.\n");
  }

  if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
    printf("Error reading fixed information.\n");
  }

  fbp = (char *) mmap(0,
                      finfo.smem_len,
                      PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
  if ((int) fbp == -1) {
    printf("Failed to mmap.\n");
  }
}

void fbclose()
{
  munmap(fbp, finfo.smem_len);
  if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_vinfo)) {
    printf("Error resetting variable screen information.\n");
  }
  close(fbfd);
}

int maintemp()
{
  short p[]={35,23,52,35,74,162,57,34,241,41};
  fbopen();
  color = 0xff;
  draw_line(0, 0, 400, 100);
  color = 0xa0a000;
  draw_circle(60, 60, 10);
  fill_circle(407, 317, 34);
  color = 127;
  drawgrid();
  color=0xff0000;
  fblines(p,5);
  sleep(5);
  fbclose();
  return (0);
}

/* fbgraph.c  2020.12.10 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>

char *fbp = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
int colr;

void put_pixeli1(int x, int y, int c)
{
  unsigned int pix_offset = 4 * x + y * finfo.line_length;
  *((char *) (fbp + pix_offset)) = c;
}

void put_pixel(int x, int y, int c)
{
  * (int *) (fbp + 4 * x + y * finfo.line_length) = colr;
}

void draw()
{
  int x, y;

  memset(&colr, 255, 1);

  memset(fbp, 1, vinfo.xres * vinfo.yres); /* blue backgnd */

  for (y = 0; y < (vinfo.yres); y += 10) {
    for (x = 0; x < vinfo.xres; x++) {
      put_pixel(x, y, 15);
    }
  }

  for (x = 0; x < vinfo.xres; x += 10) {
    for (y = 0; y < (vinfo.yres); y++) {
      put_pixel(x, y, 15);
    }
  }

  int n;
  n = (vinfo.xres < vinfo.yres) ? vinfo.xres : vinfo.yres;
  for (x = 0; x < n; x++) {
    put_pixel(x, x, 4); /* red line */
  }

}

int main(int argc, char *argv[])
{
  int fbfd = 0;
  struct fb_var_screeninfo orig_vinfo;
  long int screensize = 0;

  fbfd = open("/dev/fb0", O_RDWR);
  if (fbfd == -1) {
    printf("Error: cannot open framebuffer device.\n");
    return (1);
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

  screensize = finfo.smem_len;
  fbp = (char *) mmap(0,
                      screensize,
                      PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

  if ((int) fbp == -1) {
    printf("Failed to mmap.\n");
  } else {
    draw();
    sleep(5);
  }

  munmap(fbp, screensize);
  if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_vinfo)) {
    printf("Error resetting variable screen information.\n");
  }
  close(fbfd);
  return 0;
}

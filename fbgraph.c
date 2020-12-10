/* fbgraph.c  2020.12.10 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

char *fbp = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
int color;

void putpixel(int x, int y)
{
  * (int *) (fbp + 4 * x + y * finfo.line_length) = color;
}

void draw()
{
  int x, y;
  color=255<<0;

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

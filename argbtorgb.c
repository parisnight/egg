/* removes third byte argb -> rgb 2020.12.19 */
/* used to convert raw framebuffer into something suitable for gimp */

#include <stdio.h>

void main(int argc, char **argv)
{
  FILE *ifil, *ofil;
  int i = 0, j, c, b[4];

  ifil = fopen(argv[1], "rb");
  ofil = fopen("out", "wb");
/*while ((c=getc(ifil))!=EOF)
 if ((i++ %4) != 3) putc(c,ofil);
*/

  while ((c = getc(ifil)) != EOF) {
    if ((i++ % 4) == 3)
      for (j = 0; j < 3; j++)
        putc(b[j], ofil);
    else
      b[3 - (i % 4)] = c;       /* swap byte ordering */
  }

}

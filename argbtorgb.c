/* removes third byte argb -> rgb 2020.12.19 */

#include <stdio.h>

void main(int argc, char **argv) {
FILE *ifil,*ofil;
int i=0,c;

ifil=fopen(argv[1],"rb");
ofil=fopen("out","wb");
while ((c=getc(ifil))!=EOF)
 if ((i++ %4) != 3) putc(c,ofil);

}



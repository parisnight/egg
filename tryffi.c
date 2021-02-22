/* gcc -shared -o libbessel.so -fPIC my.c */

#include <math.h>
#include <libguile.h>

int i, v[]={3,1,2,3,4,5,3,6,5,8};
char h[]="hello";

SCM
j0_wrapper (SCM x)
{
  printf("%s\n",h);
  for (i=0; i<10; i++) printf("%d\n",v[i]);
  return scm_from_double (j0 (scm_to_double (x)));
}

void
init_math_bessel ()
{
  scm_c_define_gsubr ("j0", 1, 0, 0, j0_wrapper);
}

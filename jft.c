/* gcc -lm -ljack -lfftw3 jft.c   2020.1 */

#include <stdlib.h>
#include <math.h>
#include <fftw3.h>

#include <jack/jack.h>
#include <sys/fcntl.h>          /* for O_RDWR */
#include <math.h>               /* exp2f */
#include <string.h>	        /* strcpy */
#include <unistd.h>		/* read, sleep */

jack_client_t *client;
jack_port_t *inputport[2];
int srate=1;                    /* global sampling rate 1=jack not initialized yet */
int cycle=1, nave=5;
extern int color;

#define N 512
fftw_complex in[N],  out[N], in2[N]; /* double [2] */
double ind[N];
fftw_plan p, q;
short pt[5000], ptold[5000];
short zeroline[300];
int xr=500,yo=500,yoffset=0;

short ytransform(double r, double i) {
  double d;
  d = (r*r + i*i);
  if (d>0) d=10*log10(d); else d= -200;
  if (d<-200) d=-200;
  return(yo-d+yoffset);
}

void grid() {
  int i,j;
  for (i=j=0; j<6; i+=8, j++) {
    zeroline[i]=zeroline[i+6]=0;
    zeroline[i+1]=zeroline[i+3]=yo+j*20;
    zeroline[i+2]=zeroline[i+4]=xr;
    zeroline[i+5]=zeroline[i+7]=yo+(j+1)*20-10;
  }
  for (i=0; i<20; i+=1) printf("%d ",zeroline[i]); 
}


int process (jack_nframes_t nframes, void *arg) {
  jack_position_t pos;
  jack_default_audio_sample_t *inport[2];
  int i, j;
  
  inport[0]=(jack_default_audio_sample_t *) jack_port_get_buffer(inputport[0],nframes);
//  inport[1]=(jack_default_audio_sample_t *) jack_port_get_buffer(inputport[1],nframes);

  for (i = 0; i < N; i++) {
    in[i][0] = inport[0][i];
    in[i][1] = 0;
    ind[i] = inport[0][i];
  }

/*    color=0;
  fblines(pt,N);
*/
  fftw_execute(q);
  for (i = 0; i < N; i++ ) {
    pt[2*i]=i;
    pt[2*i+1]= ((cycle-1)*pt[2*i+1] + ytransform(out[i][0], out[i][1]))/cycle;
    //pt[2*i+1]= ytransform(out[i][0], out[i][1]);
  }
/*    color=0xaaaaaaaa;
  fblines(pt,N);
*/
if (cycle++ > nave) {
    cycle=1;
    color=0;
    fblines(ptold,N);
    color=0x808080;
    fblines(zeroline,24);
    color=0x0000ff;
    fblines(zeroline,2);
    color=0xaaaaaa;
    fblines(pt,N);
    for (i = 0; i < 2*N; i++) { ptold[i]=pt[i]; pt[i]=0; }
  }



/*    printf("\033[s \033[0;0H \033[33m  %-8.0f %8d %d %40d %8d\033[u", 20.0f * log10f(peak), npeak,
	   sampleactive, pos.frame/converttoframes, pos.frame/srate);
  fflush(stdout);
*/
  return(0);
}


void jack_init () {
  client = jack_client_open("jft", JackNullOption, NULL);
  srate = jack_get_sample_rate (client);
  jack_set_process_callback(client, process, 0);
  inputport[0] = jack_port_register(client, "in1",JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput, 0);
  inputport[1] = jack_port_register(client, "in2",JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput, 0);
  jack_activate(client);
}

#define EXIT 999
#define nmenu 1
#define nchoice 9
char *menu[nmenu][nchoice] = {
  {
   "?",
   "exit",
   "%",
   "!",
   "pipe       s",
   "r",
   "ssampleactive     d",
   "p",
   "nframes"
  }
};

int choice (int m, char *s, FILE *fil) {
  int i=0, j;
  char c,inp[20];
  do {
    //printf ("%s", s);
    do {
      c=fgetc(fil);
      if (c=='\n') return(EXIT);
    } while (c==0x20);
    do {
      inp[i++]=c;
      c=fgetc(fil);
    } while (c!=0x20 && c!='\n');
    if (c=='\n') ungetc('\n',fil); /* ensure EXIT next time choice called */
    /*printf("%02x %02x %02x\n",inp[0],inp[1],inp[2]);*/
    inp[i] = 0;                    /* terminate string with null */
    for (i = 0; i < nchoice && strncmp (inp, menu[m][i], 2); i++);
    if (i == 0)
      for (j = 1; j < nchoice && *menu[m][j] != 0; j++)
	printf (" %s\n", menu[m][j]);
    else if (i == nchoice)
      printf (" Unknown command\n");
  } while (i == nchoice);
  return (i);
}

int main (int argc, char **argv) {
  int i, j, mipo, cmd, stakptr=0; /* no static vars need initialization */
  char str[80], path[80], fil[80];
  float f;
  FILE *cfil, *stak[10];
  
  fbopen();
    color=0x808080;
grid();
    fblines(zeroline,24);
//exit(0);

  p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);
  q = fftw_plan_dft_r2c_1d(N, ind, out, FFTW_ESTIMATE);
  jack_init();

  path[0]=0;
  cfil=stak[0]=stdin; 
  while (1) {
  printf("> ");
  while ((cmd=choice (0, "> ", cfil)) != EXIT) {
    switch (cmd) {
    case 1: exit (-1);
    case 2:fgets(str, 80, cfil); break;
    case 3:
      fgets(str, 60, cfil); str[strlen(str)-1]=0;
      system(str);
      break;
    case 4:			/* pipe */
      fscanf(cfil,"%s",str);
      if (!strcmp(str,"stdin")) { /* pop */
	if (stakptr>0) {
	  fclose (stak[stakptr]);
	  cfil= stak[--stakptr];
	} else puts("pi stak error");
      } else cfil=stak[++stakptr]=fopen(str,"rt");
      break;

    case 5: break;
    case 6: fscanf(cfil, "%d", &i); nave=i; break;
    case 7: fscanf(cfil, "%d", &yoffset); break;
    case 8: break;
    case 9: break;
    }
  }
  }
  return(0);
}


/* gcc -lm -ljack -lfftw3 fbgraph.c jft.c   2020.1 */

#include <stdlib.h>
#include <math.h>
#include <fftw3.h>
#include <jack/jack.h>
#include <string.h>	        /* strcmp */

extern int color;
jack_client_t *client;
jack_port_t *inputport[2];
int srate, mode=1, cycle=1, nave=5, xr=500,yo=500,yoffset=0;
short zeroline[300];

#define N 1024
double ind[N], acc[N], window[N];
fftw_complex out[N]; /* double [2] */
fftw_plan q;
short pt[2*N], ptold[2*N];

double mag(double r ,double i) {
  return(r*r+i*i);
}

double db(double d) {
  if (d>0) d=10*log10(d); else d= -200;
//  if (d<-200) d= -200;
  return(d);
}

void grid() {
  int i,j;
  for (i=j=0; j<6; i+=8, j++) {
    zeroline[i]=zeroline[i+6]=0;
    zeroline[i+1]=zeroline[i+3]=yo+j*20;
    zeroline[i+2]=zeroline[i+4]=xr;
    zeroline[i+5]=zeroline[i+7]=yo+(j+1)*20-10;
  }
  /*for (i=0; i<20; i+=1) printf("%d ",zeroline[i]); */ 
}

void fourier (jack_default_audio_sample_t *inport[]) {
  int i, j;
  for (i = 0; i < N; i++) {
    ind[i] = inport[0][i] * window[i] * window[i];
  }

  fftw_execute(q);
  for (i = 0; i < N; i++ ) {
    acc[i]= ((cycle-1)*acc[i] + mag(out[i][0], out[i][1]))/cycle;
  }

  if (cycle++ > nave) {
    cycle=1;
    for (i = 0; i < N; i++ ) {
      pt[2*i]=i;
      pt[2*i+1]= yo + yoffset - db(acc[i]);
    }
    color=0;
    fblines(ptold,N);
    color=0x808080;
    fblines(zeroline,24);
    color=0xaaaaaa;
    fblines(pt,N);
    for (i = 0; i < 2*N; i++) ptold[i]=pt[i];
  }
}

void oscilloscope (jack_default_audio_sample_t *inport[]) {
  int i;
  color=0;
  fblines(pt,N);
  for (i = 0; i < N; i++ ) {
    pt[2*i]=i;
    pt[2*i+1]= yo-250*inport[0][i];
  }
  color=0x808080;
  fblines(pt,N);
}

int process (jack_nframes_t nframes, void *arg) {
  jack_position_t pos;
  jack_default_audio_sample_t *inport[2];
  inport[0]=(jack_default_audio_sample_t *) jack_port_get_buffer(inputport[0],nframes);
  if (mode) fourier(inport);
  else oscilloscope(inport);
  return(0);
}

void jack_init () {
  client = jack_client_open("jft", JackNullOption, NULL);
  srate = jack_get_sample_rate (client);
  jack_set_process_callback(client, process, 0);
  inputport[0] = jack_port_register(client, "in1",JACK_DEFAULT_AUDIO_TYPE,JackPortIsInput, 0);
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
   "mode d",
   "nave    d",
   "yoffset d",
   "clear"
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
  
  q = fftw_plan_dft_r2c_1d(N, ind, out, FFTW_ESTIMATE);
  grid();
  fbopen();
  jack_init();
  for (i=0; i<N; i++) { /* parzen, hamming */
    //window[i] = 1 - fabs((i-(N-1)/2.0)*2/(N+1));
    window[i] = 0.54 - 0.46 * cos(6.28  * i / N);
  }
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

    case 5: fscanf(cfil, "%d", &mode); break;
    case 6: fscanf(cfil, "%d", &nave); break;
    case 7: fscanf(cfil, "%d", &yoffset); break;
    case 8: system("clear"); break;
    }
  }
  }
  return(0);
}


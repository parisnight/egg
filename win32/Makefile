VPATH = ../../include
VPATH += ../../samples

CC = gcc
CFLAGS = -Wall -O2 

SAMPLES = ezx_test.exe ezx_test2.exe ezx_test3.exe ezx_test4.exe ezx_test5.exe ezx_test6.exe ezx_test7.exe ezx_test8.exe ezx_test9.exe ezx_test10.exe ezx_test11.exe ezx_test12.exe ezx_test13.exe tsp.exe tsp_LK.exe 3d_clock.exe ezx_engine.exe ezx_rotary.exe ezx_life.exe
SAMPLES_OBJS = $(SAMPLES:.exe=.o)

all : libezx.a 

samples : $(SAMPLES)

libezx.a : ezxdisp.o ezx_main.o
	ar -rs $@ $^

ezxdisp.o : ezxdisp.c ezxdisp.h
	$(CC) -I../../include $(CFLAGS) -c $<

ezx_main.o : ezx_main.c
	$(CC) $(CFLAGS) -c $< 

$(SAMPLES) : libezx.a
$(SAMPLES_OBJS) : ezxdisp.h

%.exe : %.o
	$(CC) $(CFLAGS) $< -o $@ -L. -lmingw32 -lezx -mwindows

%.o : %.c
	$(CC) -I../../include $(CFLAGS) -c $<

clean :
	rm -f *.o *.a $(SAMPLES)


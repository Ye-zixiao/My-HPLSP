CC := gcc
ROOT := ../..
INCLUDE := $(ROOT)/include
LIBPATH := $(ROOT)/lib

DLFLAGS := -Wl,-rpath $(LIBPATH) -L$(LIBPATH) -lmyunp -lpthread
CFLAGS := -Wall -O -I$(INCLUDE)

SRCS := $(wildcard *.c)
OBJS := $(SRCS:c=o)
MAIN := main


.PHONY:
all: LIBMYUNP $(MAIN)

LIBMYUNP:
	cd $(LIBPATH) && make

$(MAIN):$(OBJS)
	$(CC) -o $(MAIN) $(OBJS) $(DLFLAGS) $(CFLAGS)
	
$(OBJS):%.o:%.c

clean: rmobj
	-rm $(MAIN)

rmobj:
	-rm $(OBJS)

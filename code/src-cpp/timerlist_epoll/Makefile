CC := g++
ROOT := ../..
INCLUDE := $(ROOT)/include
LIBPATH := $(ROOT)/lib

DLFLAGS := -Wl,-rpath $(LIBPATH) -L$(LIBPATH) -lmyunp -lpthread
CFLAGS := -Wall -O -I$(INCLUDE)

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:cpp=o)
MAIN := main


.PHONY:
all: LIBMYUNP $(MAIN)

LIBMYUNP:
	cd $(LIBPATH) && make -j4

$(MAIN):$(OBJS)
	$(CC) -o $(MAIN) $(OBJS) $(DLFLAGS) $(CFLAGS)
	
$(OBJS):%.o:%.cpp
	$(CC) -c -o $@ $< $(CFLAGS) 

clean: rmobj
	-rm $(MAIN)

rmobj:
	-rm $(OBJS)

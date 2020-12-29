CC := g++
ROOT := ..
INCLUDE := $(ROOT)/include
LIBPATH := $(ROOT)/lib

DFLAGS := -Wl,-rpath $(LIBPATH) -L$(LIBPATH) -lmyunp -lpthread
CFLAGS := -I$(INCLUDE) -O -Wall

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:cpp=o)
PRGS := $(SRCS:.cpp=)

.PHONY:
all: LIBUNP $(PRGS)

LIBUNP:
	cd $(LIBPATH) && make -j4

$(PRGS):%:%.o
	$(CC) -o $@ $< $(DFLAGS)

$(OBJS):%.o:%.cpp
	$(CC) -o $@ -c $< $(CFLAGS)

clean: rmobj
	-rm $(PRGS)

rmobj:
	-rm $(OBJS)

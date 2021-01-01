CC := g++
ROOT := ..
INCLUDE := $(ROOT)/include
CLIBPATH := $(ROOT)/lib
CPPLIBPATH := $(ROOT)/lib-cpp

DFLAGS := -Wl,-rpath $(CLIBPATH) -L$(CLIBPATH) -lmyunp -Wl,-rpath $(CPPLIBPATH) \
	-L$(CPPLIBPATH) -lmyunpc++ -lpthread
CFLAGS := -I$(INCLUDE) -O -Wall

SRCS := $(wildcard *.cpp)
OBJS := $(SRCS:cpp=o)
PRGS := $(SRCS:.cpp=)

.PHONY:
all: LIBUNP $(PRGS)

LIBUNP:
	cd $(CLIBPATH) && make -j4
	cd $(CPPLIBPATH) && make -j4

$(PRGS):%:%.o
	$(CC) -o $@ $< $(DFLAGS)

$(OBJS):%.o:%.cpp
	$(CC) -o $@ -c $< $(CFLAGS)

clean: rmobj
	-rm $(PRGS)

rmobj:
	-rm $(OBJS)

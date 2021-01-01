CC := g++
ROOT := ..
INCLUDE := $(ROOT)/include

DFLAGS := -shared -lpthread
CFLAGS := -fPIC -O -Wall -I$(INCLUDE)
OBJS := $(patsubst %.cpp,%.o,$(wildcard *.cpp))
LIBUNP := libmyunpc++.so

.PHONY:
$(LIBUNP): $(OBJS)
	$(CC) -o $(LIBUNP) $(OBJS) $(DFLAGS)

$(OBJS):%.o:%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<

clean: rmobj
	-rm $(LIBUNP)

rmobj:
	-rm $(OBJS)

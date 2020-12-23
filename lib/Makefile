CC := gcc
ROOT := ..
INCLUDE := $(ROOT)/include

DFLAGS := -shared -lpthread
CFLAGS := -fPIC -O -Wall -I$(INCLUDE)
OBJS := $(patsubst %.c,%.o,$(wildcard *.c))
LIBUNP := libmyunp.so

.PHONY:
$(LIBUNP): $(OBJS)
	$(CC) -o $(LIBUNP) $(OBJS) $(DFLAGS)

$(OBJS):%.o:%.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean: rmobj
	-rm $(LIBUNP)

rmobj:
	-rm $(OBJS)

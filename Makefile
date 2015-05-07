TARGET = server-demo
OBJS = $(patsubst %.c,%.o,$(wildcard *.c hash/*.c))
CC = gcc
LINK = gcc
override CFLAGS = -Wall -D__WITH_MURMUR -O3 -g -lpthread -lrt

CCCOLOR = "\033[34m"
LINKCOLOR = "\033[34;1m"
SRCCOLOR = "\033[33m"
BINCOLOR = "\033[32m"
MAKECOLOR = "\033[32;1m"
ENDCOLOR = "\033[0m"
QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)$(CC)$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR);
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)$(CC)$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR);

$(TARGET) : $(OBJS)
	$(QUIET_LINK) $(LINK) $(CFLAGS) $(OBJS) -o $@

%.o : %.c
	$(QUIET_CC) $(CC) -c $(CFLAGS) $< -o $@

dep:
	$(CC) -MM $(CFLAGS) *.c > Makefile.dep

clean :
	rm -rf $(TARGET) $(OBJS)

all :
	$(TARGET)

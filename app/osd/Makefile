
TARGET = osd
CPPSRCS = osd_app.c

INCLUDE = -I$(DRVPATH)
OBJS = $(CPPSRCS:.c=.o)


all:$(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	rm -f *$(TARGET) $(OBJS)

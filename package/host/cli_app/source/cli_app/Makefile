CC=gcc
RM = rm -f
LDFLAGS=-pthread -lm
OBJS = cli_cmd.o cli_netlink.o cli_util.o main.o
TARGET = cli_app

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(TARGET)
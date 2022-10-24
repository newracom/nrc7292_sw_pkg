CC=gcc -g
RM = rm -f
LDFLAGS=-lcurl -lcjson
OBJS = nrc_fw_updater.o
TARGET = nrc_fw_updater

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

clean:
	$(RM) $(OBJS) $(TARGET) *~

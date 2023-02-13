CC ?= gcc
AR ?= ar
RM ?= rm -f

TARGET = cli_app

LIB_SRCS = cli_cmd.c cli_netlink.c cli_util.c
LIB_OBJS = cli_cmd.o cli_netlink.o cli_util.o

EXE_SRCS = main.c
EXE_OBJS = main.o

CFLAGS += -pthread
LFLAGS += -L./ -l$(TARGET) -lm

all: lib$(TARGET).a $(TARGET)

lib$(TARGET).a: $(LIB_SRCS)
	@echo "$@"
	@$(CC) -c $^
	@$(AR) rc $@ $(LIB_OBJS)
	@$(RM) *.o

lib$(TARGET).so: $(LIB_SRCS)
	@echo "$@"
	@$(CC) -fPIC -c $^
	@$(CC) -shared -o $@ $(LIB_OBJS)
	@$(RM) *.o

$(TARGET): $(EXE_SRCS) lib$(TARGET).a
	@echo "$@"
	@$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)

lib_test: lib_test.c
	@echo "$@"
	@$(CC) $^ -o $@ $(CFLAGS) $(LFLAGS)

clean:
	@$(RM) -v lib$(TARGET).* $(TARGET) lib_test

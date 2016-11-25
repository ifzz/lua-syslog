CFLAGS += -fPIC -O2
CPPFLAGS += -Isrc
LDFLAGS += -O2

CFLAGS += $(shell pkg-config lua5.3 --cflags-only-other)
CPPFLAGS += $(shell pkg-config lua5.3 --cflags-only-I)
LDFLAGS += $(shell pkg-config lua5.3 --libs-only-L)
LDFLAGS += $(shell pkg-config lua5.3 --libs-only-other)
LDLIBS += $(shell pkg-config lua5.3 --libs-only-l)

lib_objs := \
  src/lua_syslog.o

syslog.so:LDFLAGS += --retain-symbols-file syslog.map
syslog.so: $(lib_objs)
	$(LD) $(LDFLAGS) -shared -o syslog.so $(lib_objs) $(LDLIBS)

%.o: %.c src/lua_syslog.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

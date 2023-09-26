# libjuice

NAME=libjuice
CC=$(CROSS)gcc
AR=$(CROSS)ar
RM=rm -f
CFLAGS=-O2 -pthread -fPIC -Wno-address-of-packed-member
LDFLAGS=-pthread
LIBS=

INCLUDES=-Iinclude/juice
LDLIBS=

USE_NETTLE ?= 0
ifneq ($(USE_NETTLE), 0)
        CFLAGS+=-DUSE_NETTLE=1
        LIBS+=nettle
else
        CFLAGS+=-DUSE_NETTLE=0
endif

NO_SERVER ?= 0
ifneq ($(NO_SERVER), 0)
        CFLAGS+=-DNO_SERVER
endif

FORCE_M32 ?= 0
ifneq ($(FORCE_M32), 0)
        CFLAGS+= -m32
        LDFLAGS+= -m32
endif

CFLAGS+=-DJUICE_EXPORTS

ifneq ($(LIBS), "")
INCLUDES+=$(if $(LIBS),$(shell pkg-config --cflags $(LIBS)),)
LDLIBS+=$(if $(LIBS), $(shell pkg-config --libs $(LIBS)),)
endif

SRCS=$(shell printf "%s " src/*.c)
OBJS=$(subst .c,.o,$(SRCS))

TEST_SRCS=$(shell printf "%s " test/*.c)
TEST_OBJS=$(subst .c,.o,$(TEST_SRCS))

all: $(NAME).a $(NAME).so tests

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -MMD -MP -o $@ -c $<

test/%.o: test/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -Iinclude -Isrc -MMD -MP -o $@ -c $<

-include $(subst .c,.d,$(SRCS))

$(NAME).a: $(OBJS)
	$(AR) crf $@ $(OBJS)

$(NAME).so: $(OBJS)
	$(CC) $(LDFLAGS) -shared -o $@ $(OBJS) $(LDLIBS)

tests: $(NAME).a $(TEST_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(TEST_OBJS) $(LDLIBS) $(NAME).a

clean:
	-$(RM) include/juice/*.d *.d
	-$(RM) src/*.o src/*.d
	-$(RM) test/*.o test/*.d

dist-clean: clean
	-$(RM) $(NAME).a
	-$(RM) $(NAME).so
	-$(RM) tests
	-$(RM) include/*~
	-$(RM) src/*~
	-$(RM) test/*~


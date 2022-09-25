# Copyright 2022 Mitchell Kember. Subject to the MIT License.

define usage
Targets:
	all    Build eva
	help   Show this help message
	check  Run before committing
	test   Run tests
	clean  Remove build output

Variables:
	DEBUG  If nonempty, build in debug mode
endef

.PHONY: all help check test clean

CFLAGS := $(shell cat compile_flags.txt) $(if $(DEBUG),-O0 -g,-O3 -DNDEBUG)
DEPFLAGS = -MMD -MP -MF $(@:.o=.d)
LDFLAGS := $(if $(DEBUG),,-O3)
LDLIBS := -lreadline

src_existing := $(wildcard src/*.c)
src_gen := src/prelude.c
src := $(src_existing) $(src_gen)

obj := $(src:src/%.c=out/obj/%.o)
dep := $(obj:.o=.d)
bin := out/eva

.SUFFIXES:

all: $(bin)

help:
	$(info $(usage))
	@:

check: all test

test: $(bin)
	./test.sh

clean:
	rm -f $(src_gen)
	rm -rf out
	./test.sh clean

src/prelude.c: gen-prelude.sh src/prelude.scm
	./$^ $@

out out/obj:
	mkdir -p $@

out/obj/%.o: src/%.c | out/obj
	$(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(bin): $(obj) | out
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

-include $(dep)

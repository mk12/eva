# Compiler
CC := clang
OFLAGS := -O3
CFLAGS := -std=c11 -Weverything -pedantic -Wno-padded -Wno-switch-enum \
	-Wno-format-nonliteral
LDFLAGS := -lreadline

# Project
NAME := eva
SRC_DIR := src
OBJ_DIR := build
BIN_DIR := bin

# Files
SRCS := $(wildcard $(SRC_DIR)/*.c) src/prelude.c
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
EXEC := $(BIN_DIR)/$(NAME)

# Dependencies
DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJ_DIR)/$*.d

# Targets
.PHONY: all release debug test prep clean

all: release

release: CFLAGS += $(OFLAGS) -DNDEBUG
release: LDFLAGS += $(OFLAGS)
release: prep $(EXEC)
release:
	@echo "Target release is up to date"

debug: CFLAGS += -g
debug: prep $(EXEC)
debug:
	@echo "Target debug is up to date"

test: $(EXEC)
	@bash test.sh

prep:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

clean:
	bash test.sh clean
	rm -f src/prelude.c $(OBJS) $(DEPS) $(EXEC)

$(EXEC): $(OBJS)
	@echo "\033[0;31mLinking executable $(NAME)\033[0m"
	@$(CC) $(LDFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "\033[0;32mCC\033[0m $<"
	@$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

src/prelude.c: src/prelude.scm
	@echo "\033[1;35mGenerating $^\033[0m"
	@bash generate.sh

-include $(DEPS)

# Compiler
CC := clang
OPTIMIZE := -O3
CFLAGS := -std=c11 -Weverything -pedantic -Wno-padded -Wno-switch-enum \
	-Wno-format-nonliteral
LDFLAGS := -lreadline

# Project
NAME := eva
SRC_DIR := src
OBJ_DIR := build
BIN_DIR := bin

# Files
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
EXEC := $(BIN_DIR)/$(NAME)

# Dependencies
DEPFLAGS = -MT $@ -MMD -MP -MF $(OBJ_DIR)/$*.d

# Targets
.PHONY: all release debug test prep clean

all: release

release: CFLAGS += $(OPTIMIZE) -DNDEBUG
release: LDFLAGS += $(OPTIMIZE)
release: prep $(EXEC)

debug: CFLAGS += -g
debug: prep $(EXEC)

test:
	@echo "Test"

prep:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

clean:
	rm -f $(OBJS) $(DEPS) $(EXEC)

$(EXEC): $(OBJS)
	@echo "\033[0;31mLinking executable $(NAME)\033[0m"
	@$(CC) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "\033[0;32mCC\033[0m $<"
	@$(CC) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

-include $(DEPS)

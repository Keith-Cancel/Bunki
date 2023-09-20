define NEW_LINE


endef
CC       := gcc
CFLAGS   := -O2 -falign-functions -march=native
LDFLAGS  :=
LDLIBS   :=
LIBS     :=
INCLUDES := -I./include
C_WARN   := -pedantic-errors -Wall -Wextra -Wparentheses -Wdouble-promotion \
-Warith-conversion -Wduplicated-branches -Wduplicated-cond -Wshadow \
-Wunsafe-loop-optimizations -Wbad-function-cast -Wunsuffixed-float-constants \
-fanalyzer -Wanalyzer-too-complex

SRC_DIR  := src
OBJ_DIR  := obj

SRC_C    := $(wildcard $(SRC_DIR)/*.c)
SRC_ASM  :=
SRC      := $(SRC_C) $(SRC_ASM)

ifneq (,$(findstring x86_64, $(shell $(CC) -dumpmachine)))
	INCLUDES += -I./include/x86_64
	SRC_C    += $(wildcard $(SRC_DIR)/x86_64/*.c)
	SRC_ASM  += $(wildcard $(SRC_DIR)/x86_64/*.S)
endif

ifneq (,$(findstring aarch64, $(shell $(CC) -dumpmachine)))
	INCLUDES += -I./include/aarch64
	SRC_C    += $(wildcard $(SRC_DIR)/aarch64/*.c)
	SRC_ASM  += $(wildcard $(SRC_DIR)/aarch64/*.S)
endif

ifneq (,$(findstring arm, $(shell $(CC) -dumpmachine)))
	INCLUDES += -I./include/arm
	SRC_C    += $(wildcard $(SRC_DIR)/arm/*.c)
	SRC_ASM  += $(wildcard $(SRC_DIR)/arm/*.S)
endif

$(info INCLUDES = $(INCLUDES))
$(info SRC_C    = $(SRC_C))
$(info SRC_ASM  = $(SRC_ASM))

OBJ_C    := $(SRC_C:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJ_ASM  := $(SRC_ASM:$(SRC_DIR)/%.S=$(OBJ_DIR)/%.o)
OBJ      := $(OBJ_C) $(OBJ_ASM)
OBJ_DIRS := $(sort $(dir $(OBJ)))

$(info OBJ      = $(OBJ))
$(info OBJ_DIRS = $(OBJ_DIRS)$(NEW_LINE))
$(info Beginning Build)

.PHONY: all clean

all: ./build $(OBJ_DIRS) libbunki.a

libbunki.a: $(OBJ)
	@echo "Building Library: $@"
	ar -r ./$@ $^
	cp ./$@ ./build/lib/$@
	cp ./include/bunki.h ./build/include/bunki.h
	@echo ""

# include all the new rules that depend on the header
-include $(OBJ_C:.o=.d)

$(OBJ_C): $(SRC_C)
	@echo "Compiling: $(@:$(OBJ_DIR)/%.o=$(SRC_DIR)/%.c)"
	$(CC) -MMD $(C_WARN) $(CFLAGS) $(INCLUDES) -c $(@:$(OBJ_DIR)/%.o=$(SRC_DIR)/%.c) -o $@
	@echo ""

$(OBJ_ASM): $(SRC_ASM)
	@echo "Compiling: $(@:$(OBJ_DIR)/%.o=$(SRC_DIR)/%.S)"
	$(CC) $(CFLAGS) -c $(@:$(OBJ_DIR)/%.o=$(SRC_DIR)/%.S) -o $@
	@echo ""

clean:
	rm libbunki.a
	rm -rv $(OBJ_DIR)
	rm -rv ./build

./build:
	mkdir -p $@
	mkdir -p $@/include
	mkdir -p $@/lib
	@echo ""

$(OBJ_DIRS):
	mkdir -p $@
	@echo ""

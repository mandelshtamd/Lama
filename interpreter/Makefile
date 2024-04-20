CC := gcc
CFLAGS := -O3 -g -m32 -fstack-protector-all

BASE_DIR := ..
BUILD_DIR := $(BASE_DIR)/build
RUNTIME_DIR := $(BASE_DIR)/runtime
REGRESSION_DIR := $(BASE_DIR)/regression

TARGET := interpreter
TARGET_EXEC := $(BUILD_DIR)/$(TARGET)
TARGET_SRC := $(TARGET).c
TARGET_OBJ := $(BUILD_DIR)/$(TARGET).o

CFLAGS := -O3 -g -m32 -fstack-protector-all

RUNTIME_LIB := $(RUNTIME_DIR)/runtime.a

.PHONY: all clean test performance mkbuild lama_runtime

all: $(TARGET_EXEC)

$(TARGET_EXEC): $(TARGET_OBJ) | lama_runtime
	$(CC) $(CFLAGS) $< $(RUNTIME_LIB) -o $@

$(TARGET_OBJ): $(TARGET_SRC) | mkbuild
	$(CC) $(CFLAGS) -c $< -o $@

lama_runtime: 
	$(MAKE) -C $(RUNTIME_DIR)

mkbuild: 
	mkdir -p $(BUILD_DIR) 

clean: 
	$(RM) -r $(BUILD_DIR)
	$(MAKE) -C $(RUNTIME_DIR) clean
	$(MAKE) -C $(REGRESSION_DIR) clean 

test: $(TARGET_EXEC)
	$(MAKE) -C $(REGRESSION_DIR)
	$(MAKE) -C $(REGRESSION_DIR)/expressions
	$(MAKE) -C $(REGRESSION_DIR)/deep-expressions

performance: $(TARGET_EXEC)
	$(MAKE) -C $(BASE_DIR)/performance performance
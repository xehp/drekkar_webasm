

SRC_DIR = .
BUILD_DIR = .
CC = emcc
CFLAGS = -g

C_SOURCES = $(wildcard $(SRC_DIR)/*.c)
WASM_FILES = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.wasm)))

all: $(WASM_FILES)

#hello_world.wasm: hello_world.c
#	emcc hello_world.c -g -o hello_world.wasm
	
#$(BUILD_DIR)/%.wasm: %.c Makefile | $(BUILD_DIR) 
#	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/%.wasm: %.c Makefile | $(BUILD_DIR) 
	$(CC) $(CFLAGS) $< -o $@

clean:
	-rm $(BUILD_DIR)/*.wasm $(BUILD_DIR)/a.out $(BUILD_DIR)/a.out.* $(BUILD_DIR)/*.o
  


# File created by Georgios Skondras

CC = gcc
MPIC = mpicc
CFLAGS = -fopenmp
INCLUDE_DIR = ../zfp/include
LIB_DIR_GPU = $(HOME)/GPUbismZ/ThirdParty/zfp_gpu/zfp/builds/build_gpu/lib #put directory here
LIBS = -lgpuzfp -lm

BIN_DIR = bin

# Source files
SRC_GPU = test_gpu.c


# Targets
TARGETS = $(BIN_DIR)/test_gpu 

# Default target
all: $(BIN_DIR) $(TARGETS)

# Ensure the bin directory exists
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Rules for building targets

$(BIN_DIR)/test_gpu: $(SRC_GPU)
	$(CC) $(CFLAGS) -o $@ $< -I $(INCLUDE_DIR) -L $(LIB_DIR_GPU) -Wl,-rpath,$(LIB_DIR_GPU) $(LIBS) -lcudart


# Clean rule
clean:
	rm -f $(TARGETS)
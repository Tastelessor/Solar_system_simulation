SRC = src/simulation_configuration.c src/simulation_support.c src/main.c src/Task-parallelism/task_queue.c  src/Task-parallelism/worker.c
LFLAGS=-lm
#LFLAGS=-lm -L /work/z19/shared/extrae/lib -lmpitrace -lxml2
CFLAGS=-O3
#CFLAGS=-O3 -DINSTRUMENTED=1

.PHONY: archer2 local build

archer2: CC=cc
archer2: build

local: CC=cc
local: build

#archer2: CC=mpicc -cc=icc
#archer2: build
#
#local: CC=mpicc -cc=icc
#local: build

build:
	$(CC) -o cosmology $(SRC) $(CFLAGS) $(LFLAGS)



## Experiment Environments

**Compiler:** Cray clang version 11.0.4 

**MPI package:** cray-mpich/8.1.4

| Cluster         | ARCHER2                                                 |
| --------------- | ------------------------------------------------------- |
| Nodes           | 5,860 nodes: 5,276 standard memory, 584 high memory     |
| Processor       | 2× AMD EPYCTM 7742, 2.25 GHz, 64-core                   |
| Cores per node  | 128 (2× 64-core processors)                             |
| NUMA structure  | 8 NUMA regions per node (16 cores per NUMA region)      |
| Memory per node | 256 GiB (standard memory), 512 GiB (high memory)        |
| Memory per core | 2 GiB (standard memory), 4 GiB (high memory)            |
| Interconnect    | HPE Cray Slingshot, 2× 100 Gbps bi-directional per node |

## To Run

```shell
# Compile
make
# Run
sbatch submit_archer2.srun
```

If you want to profile the program, the change the following settings in the makefile:

```makefile
LFLAGS=-lm -L /work/z19/shared/extrae/lib -lmpitrace -lxml2
CFLAGS=-O3 -DINSTRUMENTED=1
```

Then

```shell
# Compile
make
# Run
sbatch submit_archer2_profile.srun
```

Note that the generated profiling results may be very large, so you might want to change the timestamp to a smaller number and keep the number of bodies not too large.

## To Configure

You can add attributes to the configuration file to configure the number of asteroids in the two asteroid belts, for example:

```txt
NUM_ASTEROIDS_IN_BELT=0
NUM_ASTEROIDS_IN_KUIPER=0
```

A concrete example can be the file 'config_solar_with_moons.txt'
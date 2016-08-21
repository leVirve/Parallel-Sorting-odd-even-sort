#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _DEBUG
#define DEBUG(format, args...) printf("[Line:%d] " format, __LINE__, ##args);
#else
#define DEBUG(args...)
#endif
#ifdef _INFO
#define INFO(format, args...) printf("[Line:%d] " format, __LINE__, ##args);
#else
#define INFO(args...)
#endif

#define bool        char
#define true        1
#define false       0
#define channel1    0
#define channel2    1
#define EVEN_PHASE  0
#define ODD_PHASE   1

#define READ_FILE   MPI_MODE_RDONLY
#define WRITE_FILE  MPI_MODE_WRONLY | MPI_MODE_CREATE

#define is_odd(x)   ((x) & 1)
#define is_even(x)  (!is_odd(x))
#define another(x)  ((x + 1) % 2)
#define swap(i, j)  int t = i; i = j; j = t;

extern bool sorted;
extern int num_procs, rank, size;

void mpi_init(int, char**);
int mpi_commu_basic(int rank, int* target, int* buffer, int channel);
void mpi_file(char* filename, int* nums, int* count, int mode);

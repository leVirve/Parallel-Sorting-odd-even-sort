#include <mpi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
#define LEFT_PHASE  0
#define RIGHT_PHASE 1
#define is_odd(x)   ((x) & 1)
#define is_even(x)  (!is_odd(x))
#define swap(i, j)  int t = i; i = j; j = t;
#define timeit(t)   clock_gettime(CLOCK_REALTIME, t)

bool sorted = false;
int world_size, world_rank, subset_size, *nums, *recv, *tmp;

long io_time = 0, comp_time = 0, comm_time = 0;
struct timespec s, e;

void calc_time(long* target, struct timespec a, struct timespec b)
{
    int sec = a.tv_sec - b.tv_sec;
    int nsec = a.tv_nsec - b.tv_nsec;
    *target += (long)sec * 1000000000 + nsec;
}

int cmp(const void* a, const void* b)
{
    /* watch out! it may overflow with return x - y */
    int x = *(int*)a, y = *(int*)b;
    return (x > y) - (x < y);
}

void mpi_read_file(char* filename, int* nums, int* count)
{
    MPI_File input;
    MPI_Status status;
    MPI_File_open(MPI_COMM_WORLD, filename,
                  MPI_MODE_RDONLY, MPI_INFO_NULL, &input);
    MPI_File_set_view(input, sizeof(int) * subset_size * world_rank,
                      MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
    timeit(&s);
    MPI_File_read_all(input, nums, subset_size, MPI_INT, &status);
    timeit(&e); calc_time(&io_time, e, s);
    MPI_File_close(&input);
    MPI_Get_count(&status, MPI_INT, count);
    DEBUG("My rank=%d/%d read_size=%d\n", world_rank, world_size, *count);
}

void mpi_write_file(char* filename, int* nums, int* count)
{
    MPI_File fh;
    MPI_Status status;
    MPI_File_open(MPI_COMM_WORLD, filename,
                  MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, sizeof(int) * subset_size * world_rank,
                      MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
    timeit(&s);
    MPI_File_write_all(fh, nums, *count, MPI_INT, &status);
    timeit(&e); calc_time(&io_time, e, s);
    MPI_File_close(&fh);
}

void _merge(int* buf, int lsz, int* recv, int rsz, bool phase)
{
    int i = 0, j = 0, k = 0, memsz, *memptr;
    timeit(&s);
    while (i < lsz || j < rsz) {
        if (i >= lsz) tmp[k++] = recv[j++];
        else if (j >= rsz) tmp[k++] = buf[i++];
        else tmp[k++] = (buf[i] <= recv[j]) ? buf[i++] : recv[j++];
    }
    memsz = lsz * sizeof(int);
    memptr = (phase == LEFT_PHASE) ? tmp : tmp + rsz;
    if (memcmp(buf, memptr, memsz) != 0) sorted = false;
    memcpy(buf, memptr, memsz);
    timeit(&e); calc_time(&comp_time, e, s);
}

void mpi_recv(int rank, int* nums, int count)
{
    if (world_rank >= world_size || rank < 0) return;
    int c;
    MPI_Status stat;
    timeit(&s);
    MPI_Recv(recv, subset_size, MPI_INT, rank, channel1, MPI_COMM_WORLD, &stat);
    MPI_Send(nums, count, MPI_INT, rank, channel2, MPI_COMM_WORLD);
    timeit(&e); calc_time(&comm_time, e, s);
    MPI_Get_count(&stat, MPI_INT, &c);
    _merge(nums, count, recv, c, RIGHT_PHASE);
}

void mpi_send(int rank, int* nums, int count)
{
    if (rank >= world_size) return;
    int c;
    MPI_Status stat;
    timeit(&s);
    MPI_Send(nums, count, MPI_INT, rank, channel1, MPI_COMM_WORLD);
    MPI_Recv(recv, subset_size, MPI_INT, rank, channel2, MPI_COMM_WORLD, &stat);
    timeit(&e); calc_time(&comm_time, e, s);
    MPI_Get_count(&stat, MPI_INT, &c);
    _merge(nums, count, recv, c, LEFT_PHASE);
}

int main(int argc, char** argv)
{
    int file_size, count;
    sscanf(argv[1], "%d", &file_size);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    /** For data size is less then processors **/
    if (file_size < world_size) world_size = file_size;

    subset_size = file_size / world_size;
    if (file_size % world_size) subset_size += 1;
    if (world_size <= 1) sorted = true;

    nums = (int*) malloc(subset_size * sizeof(int));
    recv = (int*) malloc(subset_size * sizeof(int));
    tmp = (int*) malloc(subset_size * 2 * sizeof(int));
    mpi_read_file(argv[2], nums, &count);

    timeit(&s);
    qsort(nums, count, sizeof(int), cmp);
    timeit(&e); calc_time(&comp_time, e, s);
#ifdef _TIME
    printf("world_size %d\n", world_size);
#endif
    while (!sorted) {
        sorted = true;
        /*** even-phase ***/
        if (is_even(world_rank)) mpi_send(world_rank + 1, nums, count);
        else mpi_recv(world_rank - 1, nums, count);
        MPI_Barrier(MPI_COMM_WORLD);

        /*** odd-phase ***/
        if (is_odd(world_rank)) mpi_send(world_rank + 1, nums, count);
        else mpi_recv(world_rank - 1, nums, count);
        MPI_Barrier(MPI_COMM_WORLD);

        bool tmp = sorted;
        timeit(&s);
        MPI_Allreduce(&tmp, &sorted, 1, MPI_CHAR, MPI_BAND, MPI_COMM_WORLD);
        timeit(&e); calc_time(&comm_time, e, s);
    }
    mpi_write_file(argv[3], nums, &count);
    MPI_Finalize();
    free(nums);
    free(recv);
    free(tmp);

#ifdef _TIME
    printf("#Rank%d: total=%ld nsec (io=%ld, comm=%ld, comp=%ld)\n",
           world_rank,
           io_time + comm_time + comp_time,
           io_time, comm_time, comp_time);
#endif

    return 0;
}

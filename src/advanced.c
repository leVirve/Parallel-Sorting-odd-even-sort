#include <mpi.h>
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
#define MICRO_SEC   1000000
#define channel1    0
#define channel2    1
#define LEFT_PHASE  0
#define RIGHT_PHASE   1

#define is_odd(x)   ((x) & 1)
#define is_even(x)  (!is_odd(x))
#define swap(i, j)  int t = i; i = j; j = t;

bool sorted = false;
int world_size, world_rank, subset_size, last_size;

void calc_time(long* target, struct timespec a, struct timespec b)
{
    int sec = a.tv_sec - b.tv_sec;
    int nsec = a.tv_nsec - b.tv_nsec;
    *target += sec * 1000000000 + nsec;
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
    MPI_File_read_all(input, nums, subset_size, MPI_INT, &status);
    MPI_File_close(&input);
    MPI_Get_count(&status, MPI_INT, count);
}

void mpi_write_file(char* filename, int* nums, int* count)
{
    MPI_File fh;
    MPI_Status status;
    MPI_File_open(MPI_COMM_WORLD, filename,
                  MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, sizeof(int) * subset_size * world_rank,
                      MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
    MPI_File_write_all(fh, nums, *count, MPI_INT, &status);
    MPI_File_close(&fh);
}

void _merge(int* buf, int lsz, int* recv, int rsz, bool phase)
{
    int tmp[subset_size * 2];
    int i = 0, j = 0, k = 0;
    while (i < lsz || j < rsz) {
        if (i >= lsz) tmp[k++] = recv[j++];
        else if (j >= rsz) tmp[k++] = buf[i++];
        else {
            if (buf[i] <= recv[j]) tmp[k++] = buf[i++];
            else tmp[k++] = recv[j++];
        }
    }
    if (phase == LEFT_PHASE) {
        for (i = 0; i < lsz; ++i) {
            if (buf[i] != tmp[i]) {
                sorted = false;
                buf[i] = tmp[i];
            }
        }
    } else {
        for (i = 0; i < lsz; ++i) {
            if (buf[i] != tmp[rsz + i]) {
                sorted = false;
                buf[i] = tmp[rsz + i];
            }
        }
    }
}

void mpi_recv(int rank, int* nums, int count)
{
    if (world_rank >= world_size || rank < 0) return;
    int sz = subset_size;
    int recv[subset_size];
    MPI_Status status;
    MPI_Recv(recv, sz, MPI_INT, rank, channel1, MPI_COMM_WORLD, &status);
    MPI_Send(nums, count, MPI_INT, rank, channel2, MPI_COMM_WORLD);
    _merge(nums, count, recv, sz, RIGHT_PHASE);
}

void mpi_send(int rank, int* nums, int count)
{
    if (rank >= world_size) return;
    int sz = (rank == world_size - 1) ? last_size : subset_size;
    int recv[subset_size];
    MPI_Status status;
    MPI_Send(nums, count, MPI_INT, rank, channel1, MPI_COMM_WORLD);
    MPI_Recv(recv, sz, MPI_INT, rank, channel2, MPI_COMM_WORLD, &status);
    _merge(nums, count, recv, sz, LEFT_PHASE);
}

int main(int argc, char** argv)
{
    int file_size;
    sscanf(argv[1], "%d", &file_size);

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    bool single_process = false;
    /** For data size is less then processors **/
    if (file_size < world_size) world_size = file_size;
    if (world_size <= 1) { single_process = true; sorted = true; }

    subset_size = file_size / world_size;
    last_size = subset_size;
    if (file_size % world_size) {
        subset_size += 1;
        file_size - subset_size * world_size;
    }

    int count, *nums = (int*) malloc(subset_size * sizeof(int));
    int head = subset_size * world_rank, tail = head + subset_size - 1;
    mpi_read_file(argv[2], nums, &count);

    qsort(nums, count, sizeof(int), cmp);

    int ccc = 0;
    while (!sorted) {
        if (ccc >= world_size) break;
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
        MPI_Allreduce(&tmp, &sorted, 1, MPI_CHAR, MPI_BAND, MPI_COMM_WORLD);
    }

    mpi_write_file(argv[3], nums, &count);
    MPI_Finalize();
    free(nums);
    return 0;
}

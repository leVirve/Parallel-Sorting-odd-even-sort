#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef _DEBUG
#define DEBUG(format, args...) \
            printf("[Line:%d] " format, __LINE__, ##args); fflush(stdout);
#else
#define DEBUG(args...)
#endif
#ifdef _INFO
#define INFO(format, args...) \
            printf("[Line:%d] " format, __LINE__, ##args); fflush(stdout);
#else
#define INFO(args...)
#endif

#define bool        char
#define true        1
#define false       0
#define MICRO_SEC   1000000
#define channel1    0
#define channel2    1
#define EVEN_PHASE  0
#define ODD_PHASE   1

#define is_odd(x)   ((x) % 2)
#define is_even(x)  (!is_odd(x))
#define swap(i, j)  int t = i; i = j; j = t;
#define time_diff(x) \
            x.tv_sec - start.tv_sec + \
            (double)(x.tv_usec - start.tv_usec) / MICRO_SEC
#define dump_status(jid) \
            struct timeval timestamp; \
            gettimeofday(&timestamp, NULL); \
            INFO("Worker#%d @(time: %.12f)\n", jid, time_diff(timestamp)); \
            fflush(stdout);

bool sorted = false;
int world_size, world_rank;
int subset_size;
double start_t, end_t;


void mpi_read_file(char* filename, int* nums, int* count)
{
    MPI_File input;
    MPI_Status status;
    struct timeval start;

    start_t = MPI_Wtime();
    gettimeofday(&start, NULL);

    MPI_File_open(MPI_COMM_WORLD, filename, \
                  MPI_MODE_RDONLY, MPI_INFO_NULL, &input);
    MPI_File_set_view(input, sizeof(int) * subset_size * world_rank, \
                      MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
    MPI_File_read_all(input, nums, subset_size, MPI_INT, &status);
    MPI_File_close(&input);

#ifdef _FILE_DEBUG
    dump_status(world_rank);
    end_t = MPI_Wtime();
#endif

    MPI_Get_count(&status, MPI_INT, count);
#ifdef _FILE_DEBUG
    DEBUG("My rank=%d/%d read_size=%d\n", world_rank, world_size, *count);
    INFO("Read file: %lf\n", end_t - start_t);
#endif
}

void mpi_write_file(char* filename, int* nums, int* count)
{
    MPI_File fh;
    MPI_Status status;

    MPI_File_open(MPI_COMM_WORLD, filename, \
                  MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, sizeof(int) * subset_size * world_rank, \
                      MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
    MPI_File_write_all(fh, nums, *count, MPI_INT, &status);
    MPI_File_close(&fh);
}

void _single_phase_sort(int* a, int index, int size)
{
    int i = index;
    for (; i < size - 1; i += 2)
        if (a[i] > a[i + 1]) { swap(a[i], a[i + 1]); sorted = false; }
}

void mpi_recv(int rank, int* nums)
{
    if (rank < 0) return;

    int carrier;
    MPI_Status status;
    MPI_Recv(&carrier, 1, MPI_INT, rank, channel1, MPI_COMM_WORLD, &status);
    if (carrier > nums[0]) { swap(carrier, nums[0]); sorted = false; }
    MPI_Send(&carrier, 1, MPI_INT, rank, channel2, MPI_COMM_WORLD);
}

void mpi_send(int rank, int* nums, int count)
{
    if (rank >= world_size) return;

    int carrier;
    MPI_Status status;
    MPI_Send(&nums[count - 1], 1, MPI_INT, rank, channel1, MPI_COMM_WORLD);
    MPI_Recv(&carrier, 1, MPI_INT, rank, channel2, MPI_COMM_WORLD, &status);
    nums[count - 1] = carrier;
}

int main(int argc, char** argv)
{
    struct timeval start;
    int file_size;
    sscanf(argv[1], "%d", &file_size);

    MPI_Init(&argc, &argv);
    gettimeofday(&start, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    /** For data size is less then processors **/
    if (file_size < world_size) world_size = 1;

    subset_size = file_size / world_size;
    if (file_size % world_size) subset_size += 1;

    int count, *nums = (int*) malloc(subset_size * sizeof(int));
    int first = subset_size * world_rank;
    bool single_process = false;
    mpi_read_file(argv[2], nums, &count);

    DEBUG("#%d/%d count=%d\n", world_rank, world_size, count);

    if (world_size <= 1) single_process = true;

    while (!sorted) {

        sorted = true;

        /*** even-phase ***/
        if (!single_process) {
            if (is_odd(first)) mpi_recv(world_rank - 1, nums);
            else mpi_send(world_rank + 1, nums, count);
            MPI_Barrier(MPI_COMM_WORLD);
        }
        _single_phase_sort(nums, EVEN_PHASE, count);

        /*** odd-phase ***/
        if (!single_process) {
            if (is_odd(first)) mpi_send(world_rank + 1, nums, count);
            else mpi_recv(world_rank - 1, nums);
            MPI_Barrier(MPI_COMM_WORLD);
        }
        _single_phase_sort(nums, ODD_PHASE, count);

        if (!single_process) {
            bool tmp = sorted;
            MPI_Allreduce(&tmp, &sorted, 1, MPI_CHAR, MPI_LAND, MPI_COMM_WORLD);
        }
    }

    mpi_write_file(argv[3], nums, &count);
    DEBUG("#%d leave sorting-loop(%d)\n", world_rank, count);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    free(nums);

#ifdef _FILE_DEBUG
    dump_status(world_rank);
#endif
    return 0;
}

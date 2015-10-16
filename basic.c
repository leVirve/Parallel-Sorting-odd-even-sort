#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#ifdef _DEBUG
#define DEBUG(format, args...) printf("[Line:%d] " format, __LINE__, ##args)
#else
#define DEBUG(args...)
#endif

#define bool  char
#define true  1
#define false 0
#define MICRO_SEC   1000000
#define swap(i, j) int t = i; i = j; j = t;
#define time_diff(x) x.tv_sec - start.tv_sec + \
            (double)(x.tv_usec - start.tv_usec) / MICRO_SEC
#define dump_status(jid)  struct timeval timestamp; \
            gettimeofday(&timestamp, NULL); \
            printf("Worker#%d @(time: %.12f)\n", jid, time_diff(timestamp)); \
            fflush(stdout);

double start_t, end_t;

void mpi_read_file(
    char* filename, int* nums,
    int subset_size, int world_rank, int world_size)
{
    MPI_File input;
    MPI_Status status;
    struct timeval start;

    nums = (int*) malloc(subset_size * sizeof(int));
    start_t = MPI_Wtime();
    gettimeofday(&start, NULL);

    MPI_File_open(MPI_COMM_WORLD, filename, \
                  MPI_MODE_RDONLY, MPI_INFO_NULL, &input);
    MPI_File_set_view(input, sizeof(int) * subset_size * world_rank, \
                      MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
    MPI_File_read_all(input, nums, subset_size, MPI_INT, &status);
    MPI_File_close(&input);

    dump_status(world_rank);
    end_t = MPI_Wtime();

#ifdef _DEBUG
    int count;
    MPI_Get_count(&status, MPI_INT, &count);
    DEBUG("My rank=%d/%d read_size=%d\n", world_rank, world_size, count);
    DEBUG("Read file: %lf\n", end_t - start_t);
    fflush(stdout);
    for (int i = 0; i < count; ++i)
        printf("(%d): %d\n", world_rank * subset_size + i, nums[i]);
#endif
}

void odd_even_sort(int* a, int size)
{
    bool sorted = false;
    while (!sorted) {
        sorted = true;
        for (int i = 1; i < size - 1; i += 2)
            if (a[i] > a[i + 1]) {
                swap(a[i], a[i + 1]);
                sorted = false;
            }
        for (int i = 0; i < size - 1; i += 2)
            if (a[i] > a[i + 1]) {
                swap(a[i], a[i + 1]);
                sorted = false;
            }
    }
    // for (int i = 0; i < size; ++i) printf("%d\n", a[i]);
}

int main(int argc, char** argv)
{
    int world_size, world_rank;
    int file_size, subset_size;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    sscanf(argv[1], "%d", &file_size);
    subset_size = file_size / world_size;
    if (subset_size % file_size) subset_size += 1;

    int *nums;
    mpi_read_file(argv[2], nums, subset_size, world_rank, world_size);



    MPI_Finalize();
    return 0;
}

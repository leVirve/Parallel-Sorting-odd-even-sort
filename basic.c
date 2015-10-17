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

#define bool  char
#define true  1
#define false 0
#define MICRO_SEC   1000000
#define is_odd(x) ((x) % 2)
#define is_even(x) (!is_odd(x))
#define swap(i, j) int t = i; i = j; j = t;
#define time_diff(x) \
            x.tv_sec - start.tv_sec + \
            (double)(x.tv_usec - start.tv_usec) / MICRO_SEC
#define dump_status(jid) \
            struct timeval timestamp; \
            gettimeofday(&timestamp, NULL); \
            INFO("Worker#%d @(time: %.12f)\n", jid, time_diff(timestamp)); \
            fflush(stdout);

int ccc = 0;
int world_size, world_rank;
double start_t, end_t;


int mpi_read_file(
    char* filename, int* nums,
    int subset_size)
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

    int count;
    MPI_Get_count(&status, MPI_INT, &count);
#ifdef _FILE_DEBUG
    DEBUG("My rank=%d/%d read_size=%d\n", world_rank, world_size, count);
    INFO("Read file: %lf\n", end_t - start_t);
    fflush(stdout);
    for (int i = 0; i < count; ++i)
        printf("(id%d): %d\n", world_rank * subset_size + i, nums[i]);
    fflush(stdout);
#endif
    return count;
}

void dump_result(
    char* filename, int* nums,
    int subset_size)
{
    MPI_File fh;
    MPI_Status status;

    MPI_File_open(MPI_COMM_WORLD, filename, \
                  MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, sizeof(int) * subset_size * world_rank, \
                      MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
    MPI_File_write_all(fh, nums, subset_size, MPI_INT, &status);
    MPI_File_close(&fh);
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
}

bool _single_phase_sort(int* a, int index, int size)
{
    bool sorted = true;
    for (int i = index; i < size - 1; i += 2)
        if (a[i] > a[i + 1]) {
            swap(a[i], a[i + 1]);
            sorted = false;
        }
    return sorted;
}

bool _recv(int rank, int* nums)
{
    if (rank < 0) return true;

    int recv, send;
    bool sorted = true;
    MPI_Status status;
    MPI_Recv(&recv, 1, MPI_INT, rank, 0, MPI_COMM_WORLD, &status);
    if (recv > nums[0]) {
        send = nums[0];
        nums[0] = recv;
        sorted = false;
    } else send = recv;
    MPI_Send(&send, 1, MPI_INT, rank, 1, MPI_COMM_WORLD);
    DEBUG("#%d(@%d) recv: %d, send: %d\n", rank + 1, ccc, recv, send);
    return sorted;
}

void _send(int rank, int* nums, int count)
{
    if (rank >= world_size) return;

    int send = nums[count - 1], recv;
    MPI_Status status;
    MPI_Send(&send, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);
    MPI_Recv(&recv, 1, MPI_INT, rank, 1, MPI_COMM_WORLD, &status);
    nums[count - 1] = recv;
    DEBUG("#%d(@%d) send: %d, recv: %d\n", rank - 1, ccc, send, recv);
}

int main(int argc, char** argv)
{
    struct timeval start;
    int file_size, subset_size;

    MPI_Init(&argc, &argv);
    gettimeofday(&start, NULL);

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    sscanf(argv[1], "%d", &file_size);
    subset_size = file_size / world_size;
    if (file_size % world_size) subset_size += 1;

    int *nums = (int*) malloc(subset_size * sizeof(int)), count;
    count = mpi_read_file(argv[2], nums, subset_size);

    bool p_sorted = false;
    while (!p_sorted) {

#ifdef _INFO
        /* Pass ccc > 4 */
        // if (ccc > 6) break;
#endif

        p_sorted = true;

        if (world_size <= 1) {
            odd_even_sort(nums, count);
            break;
        }

        // if (ccc > 5) break;

        // even-phase
        if (is_odd(subset_size)) {
            if (is_odd(world_rank))
                p_sorted = _recv(world_rank - 1, nums);
            else
                _send(world_rank + 1, nums, count);
        }
        p_sorted &= _single_phase_sort(nums, 0, count);
        INFO("#%d: %d even-phase\n", world_rank, ccc);
        MPI_Barrier(MPI_COMM_WORLD);

        // odd-phase
        if (is_odd(subset_size)) {
            if (is_odd(world_rank))
                _send(world_rank + 1, nums, count);
            else
                p_sorted = _recv(world_rank - 1, nums);
        } else {
            if (is_even(world_rank))
                _send(world_rank + 1, nums, count);
            else
                p_sorted = _recv(world_rank - 1, nums);
        }
        p_sorted &= _single_phase_sort(nums, 1, count);
        INFO("#%d: %d odd-phase\n", world_rank, ccc++);
        MPI_Barrier(MPI_COMM_WORLD);

        bool tmp;
        MPI_Allreduce(&p_sorted, &tmp, 1, MPI_CHAR, MPI_LAND, MPI_COMM_WORLD);
        p_sorted = tmp;

        DEBUG("#%d in %c\n", world_rank, p_sorted ? 'T' : 'F');
    }

    dump_result(argv[3], nums, subset_size);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    dump_status(world_rank);
    return 0;
}

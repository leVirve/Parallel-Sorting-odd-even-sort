#include "utils.h"

bool sorted = false;
int world_size, world_rank, subset_size;
int num;

void mpi_recv(int rank, int* nums)
{
    if (world_rank >= world_size || rank < 0) return;
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

void _single_phase_sort(int* a, int index, int size)
{
    int i = index;
    for (; i < size - 1; i += 2)
        if (a[i] > a[i + 1]) { swap(a[i], a[i + 1]); sorted = false; }
}

int main(int argc, char** argv)
{
    mpi_init(argc, argv);

    bool single_process = false;
    /** For data size is less then processors **/
    if (num < world_size) world_size = num;
    if (world_size <= 1) single_process = true;

    subset_size = num / world_size;
    if (num % world_size) subset_size += 1;
    DEBUG("subset_size=%d\n", subset_size);

    int count, *nums = (int*) malloc(subset_size * sizeof(int));
    int head = subset_size * world_rank, tail = head + subset_size - 1;
    mpi_read_file(argv[2], nums, &count);

    DEBUG("#%d/%d count=%d\n", world_rank, world_size, count);

    while (!sorted) {
        sorted = true;

        /*** even-phase ***/
        if (!single_process) {
            if (is_even(world_rank) && is_even(tail) ||
                is_even(world_rank) && is_odd(head))
                mpi_send(world_rank + 1, nums, count);
            if (is_odd(world_rank) && is_odd(head) ||
                is_odd(world_rank) && is_even(tail))
                mpi_recv(world_rank - 1, nums);
            MPI_Barrier(MPI_COMM_WORLD);
        }
        _single_phase_sort(nums, EVEN_PHASE, count);

        /*** odd-phase ***/
        if (!single_process) {
            if (is_even(world_rank) && is_odd(tail) ||
                is_odd(world_rank) && is_odd(tail))
                mpi_send(world_rank + 1, nums, count);
            if (is_odd(world_rank) && is_even(head) ||
                is_even(world_rank) && is_even(head))
                mpi_recv(world_rank - 1, nums);
            MPI_Barrier(MPI_COMM_WORLD);
        }
        _single_phase_sort(nums, ODD_PHASE, count);

        if (!single_process) {
            bool tmp = sorted;
            MPI_Allreduce(&tmp, &sorted, 1, MPI_CHAR, MPI_BAND, MPI_COMM_WORLD);
        }
    }
    mpi_write_file(argv[3], nums, &count);
    free(nums);
    DEBUG("#%d leave sorting-loop(%d)\n", world_rank, count);
    MPI_Finalize();
    return 0;
}

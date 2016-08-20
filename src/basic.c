#include "utils.h"

bool sorted = false;
int world_size, world_rank, subset_size;
int* nums;

void exchage_max(int rank, int* target)
{
    int buffer;
    if (mpi_commu_basic(rank, target, &buffer, channel2)) return;
    if (buffer > *target) { *target = buffer; sorted = false; }
}

void exchage_min(int rank, int* target)
{
    int buffer;
    if (mpi_commu_basic(rank, target, &buffer, channel1)) return;
    if (*target > buffer) { *target = buffer; sorted = false; }
}

void _single_phase_sort(int* a, int index, int size)
{
    for (int i = index; i < size - 1; i += 2)
        if (a[i] > a[i + 1]) { swap(a[i], a[i + 1]); sorted = false; }
}

int main(int argc, char** argv)
{
    mpi_init(argc, argv);

    int num;
    sscanf(argv[1], "%d", &num);

    if (num < world_size) world_size = num;

    bool single_process = world_size <= 1 ? true : false;

    subset_size = num / world_size;
    if (num % world_size) subset_size += 1;
    DEBUG("subset_size=%d\n", subset_size);

    int count, front = subset_size * world_rank, tail = front + subset_size - 1;
    nums = malloc(subset_size * sizeof(int));
    mpi_read_file(argv[2], nums, &count);

    DEBUG("#%d/%d count=%d\n", world_rank, world_size, count);

    while (!sorted) {
        sorted = true;

        /*** even-phase ***/
        if (!single_process) {
            if (is_even(world_rank) && is_even(tail))
                exchage_min(world_rank + 1, &nums[count - 1]);
            if (is_odd(world_rank) && is_odd(front))
                exchage_max(world_rank - 1, &nums[0]);
            MPI_Barrier(MPI_COMM_WORLD);
        }
        _single_phase_sort(nums, EVEN_PHASE, count);

        /*** odd-phase ***/
        if (!single_process) {
            if (is_odd(tail))
                exchage_min(world_rank + 1, &nums[count - 1]);
            if (is_even(front))
                exchage_max(world_rank - 1, &nums[0]);
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

#include "utils.h"

bool sorted = false;
int num_procs, rank;
int size, *nums;

void exchage_max(int proc, int* target)
{
    int buffer;
    if (mpi_commu_basic(proc, target, &buffer, channel2)) return;
    if (buffer > *target) { *target = buffer; sorted = false; }
}

void exchage_min(int proc, int* target)
{
    int buffer;
    if (mpi_commu_basic(proc, target, &buffer, channel1)) return;
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

    int count;
    nums = malloc(size * sizeof(int));
    mpi_file(argv[2], nums, &count, READ_FILE);
    DEBUG("#%d/%d count=%d\n", rank, num_procs, count);

    bool single_process = num_procs <= 1 ? true : false;
    int front = size * rank, tail = front + size - 1;

    while (!sorted) {
        sorted = true;

        /*** even-phase ***/
        if (!single_process) {
            if (is_even(rank) && is_even(tail)) exchage_min(rank + 1, &nums[count - 1]);
            if (is_odd(rank) && is_odd(front)) exchage_max(rank - 1, &nums[0]);
            MPI_Barrier(MPI_COMM_WORLD);
        }
        _single_phase_sort(nums, EVEN_PHASE, count);

        /*** odd-phase ***/
        if (!single_process) {
            if (is_odd(tail)) exchage_min(rank + 1, &nums[count - 1]);
            if (is_even(front)) exchage_max(rank - 1, &nums[0]);
            MPI_Barrier(MPI_COMM_WORLD);
        }
        _single_phase_sort(nums, ODD_PHASE, count);

        if (!single_process) {
            bool tmp = sorted;
            MPI_Allreduce(&tmp, &sorted, 1, MPI_CHAR, MPI_BAND, MPI_COMM_WORLD);
        }
    }
    DEBUG("#%d leave sorting-loop(%d)\n", rank, count);

    mpi_file(argv[3], nums, &count, WRITE_FILE);
    free(nums);

    MPI_Finalize();
}

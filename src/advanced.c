#include "utils.h"

bool sorted = false;
int num_procs, rank;
int size, *nums, *recv, *tmp;

int cmp(const void* a, const void* b)
{
    /* watch out! it may overflow with return x - y */
    int x = *(int*)a, y = *(int*)b;
    return (x > y) - (x < y);
}

void _merge(int* buf, int lsz, int* recv, int rsz, bool phase)
{
    int i = 0, j = 0, k = 0, memsz, *memptr;
    while (i < lsz || j < rsz) {
        if (i >= lsz) tmp[k++] = recv[j++];
        else if (j >= rsz) tmp[k++] = buf[i++];
        else tmp[k++] = (buf[i] <= recv[j]) ? buf[i++] : recv[j++];
    }
    memsz = lsz * sizeof(int);
    memptr = (phase == LEFT_PHASE) ? tmp : tmp + rsz;
    if (memcmp(buf, memptr, memsz) != 0) sorted = false;
    memcpy(buf, memptr, memsz);
}

void exchage_max(int proc, int* nums, int count)
{
    int cnt = mpi_commu(proc, nums, count, recv, size, channel2);
    if (cnt < 0) return;
    _merge(nums, count, recv, cnt, RIGHT_PHASE);
}

void exchage_min(int proc, int* nums, int count)
{
    int cnt = mpi_commu(proc, nums, count, recv, size, channel1);
    if (cnt < 0) return;
    _merge(nums, count, recv, cnt, LEFT_PHASE);
}

int main(int argc, char** argv)
{
    mpi_init(argc, argv);

    int count;
    nums = malloc(size * sizeof(int));
    mpi_file(argv[2], nums, &count, READ_FILE);

    recv = malloc(size * sizeof(int));
    tmp = malloc(size * 2 * sizeof(int));

    if (num_procs <= 1) sorted = true;
    qsort(nums, count, sizeof(int), cmp);

    while (!sorted) {
        sorted = true;
        /*** even-phase ***/
        if (is_even(rank)) exchage_min(rank + 1, nums, count);
        else exchage_max(rank - 1, nums, count);
        MPI_Barrier(MPI_COMM_WORLD);

        /*** odd-phase ***/
        if (is_odd(rank)) exchage_min(rank + 1, nums, count);
        else exchage_max(rank - 1, nums, count);
        MPI_Barrier(MPI_COMM_WORLD);

        bool tmp = sorted;
        MPI_Allreduce(&tmp, &sorted, 1, MPI_CHAR, MPI_BAND, MPI_COMM_WORLD);
    }

    mpi_file(argv[3], nums, &count, WRITE_FILE);
    free(nums);
    free(recv);
    free(tmp);

    MPI_Finalize();
}

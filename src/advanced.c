#include "utils.h"

bool sorted = false;
int num_procs, rank;
int size, *nums, *recv, *temp;

void _merge(int* buf, int lsz, int* recv, int rsz, bool phase)
{
    int i = 0, j = 0, k = 0, memsz, *memptr;
    while (i < lsz || j < rsz) {
        if (i >= lsz) temp[k++] = recv[j++];
        else if (j >= rsz) temp[k++] = buf[i++];
        else temp[k++] = (buf[i] <= recv[j]) ? buf[i++] : recv[j++];
    }
    memsz = lsz * sizeof(int);
    memptr = (phase == LEFT_PHASE) ? temp : temp + rsz;
    if (memcmp(buf, memptr, memsz) != 0) sorted = false;
    memcpy(buf, memptr, memsz);
}

void exchage_max(int proc, int count)
{
    int cnt = mpi_commu(proc, nums, count, recv, size, channel2);
    if (cnt < 0) return;
    _merge(nums, count, recv, cnt, RIGHT_PHASE);
}

void exchage_min(int proc, int count)
{
    int cnt = mpi_commu(proc, nums, count, recv, size, channel1);
    if (cnt < 0) return;
    _merge(nums, count, recv, cnt, LEFT_PHASE);
}

int cmp(const void* a, const void* b)
{
    int x = *(int*)a, y = *(int*)b;
    return (x > y) - (x < y);  /* watch out! may overflow with `return x - y` */
}

int main(int argc, char** argv)
{
    mpi_init(argc, argv);

    int count;
    nums = malloc(size * sizeof(int));
    recv = malloc(size * sizeof(int));
    temp = malloc(size * sizeof(int)* 2);
    mpi_file(argv[2], nums, &count, READ_FILE);

    if (num_procs <= 1) sorted = true;
    qsort(nums, count, sizeof(int), cmp);

    while (!sorted) {
        sorted = true;
        /*** even-phase ***/
        if (is_even(rank))exchage_min(rank + 1, count);
        else exchage_max(rank - 1, count);
        MPI_Barrier(MPI_COMM_WORLD);

        /*** odd-phase ***/
        if (is_odd(rank)) exchage_min(rank + 1, count);
        else exchage_max(rank - 1, count);
        MPI_Barrier(MPI_COMM_WORLD);

        bool tmp = sorted;
        MPI_Allreduce(&tmp, &sorted, 1, MPI_CHAR, MPI_BAND, MPI_COMM_WORLD);
    }

    mpi_file(argv[3], nums, &count, WRITE_FILE);
    free(nums); free(recv); free(temp);

    MPI_Finalize();
}

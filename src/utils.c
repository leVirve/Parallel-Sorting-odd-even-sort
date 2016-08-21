#include "utils.h"

void mpi_init(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int num;
    sscanf(argv[1], "%d", &num);

    num_procs = num_procs > num ? num : num_procs;
    size = (num + num_procs - 1) / num_procs;

    DEBUG("subdata size=%d\n", size);
}

int mpi_commu_basic(int proc, int* target, int* buffer, int channel)
{
    if (rank >= num_procs || proc >= num_procs || proc < 0) return -1;
    MPI_Status status;
    MPI_Sendrecv(target, 1, MPI_INT, proc, channel,
                 buffer, 1, MPI_INT, proc, another(channel),
                 MPI_COMM_WORLD, &status);
    return 0;
}

void mpi_file(char* filename, int* nums, int* count, int mode)
{
    MPI_File fh;
    MPI_Status status;
    MPI_File_open(MPI_COMM_WORLD, filename, mode, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, sizeof(int) * size * rank,
                      MPI_INT, MPI_INT, "native", MPI_INFO_NULL);

    if (mode == READ_FILE) {
        MPI_File_read_all(fh, nums, size, MPI_INT, &status);
        MPI_Get_count(&status, MPI_INT, count);
        DEBUG("rank#%d (r) count=%d of %d\n", rank, *count, size);
    } else if (mode == WRITE_FILE) {
        MPI_File_write_all(fh, nums, *count, MPI_INT, &status);
        DEBUG("rank#%d (w) count=%d\n", rank, *count);
    }
    MPI_File_close(&fh);
}

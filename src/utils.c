#include "utils.h"

void mpi_init(int argc, char** argv)
{
    sscanf(argv[1], "%d", &num);

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
}

void mpi_file_operation(int mode, char* filename, int* nums, int* count)
{
    MPI_File fh;
    MPI_Status status;
    MPI_File_open(MPI_COMM_WORLD, filename, mode, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, sizeof(int) * subset_size * world_rank,
                      MPI_INT, MPI_INT, "native", MPI_INFO_NULL);

    if (mode == READ_FILE) {
        MPI_File_read_all(fh, nums, subset_size, MPI_INT, &status);
        MPI_Get_count(&status, MPI_INT, count);
        DEBUG("rank#%d (r) count=%d of %d\n", world_rank, *count, subset_size);
    } else if (mode == WRITE_FILE) {
        MPI_File_write_all(fh, nums, *count, MPI_INT, &status);
        DEBUG("rank#%d (w) count=%d\n", world_rank, *count);
    }
    MPI_File_close(&fh);
}

void mpi_read_file(char* filename, int* nums, int* count)
{
    mpi_file_operation(READ_FILE, filename, nums, count);
}

void mpi_write_file(char* filename, int* nums, int* count)
{
    mpi_file_operation(WRITE_FILE, filename, nums, count);
}

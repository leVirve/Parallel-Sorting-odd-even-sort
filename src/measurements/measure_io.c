#include <mpi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define swap(i, j)  int t = i; i = j; j = t;
#define timeit(t)   clock_gettime(CLOCK_REALTIME, t)

int world_size, world_rank, file_size, subset_size, *nums, *tmp;

long io_time = 0, comp_time = 0, comm_time = 0;
struct timespec s, e;

void calc_time(long* target, struct timespec a, struct timespec b)
{
    int sec = a.tv_sec - b.tv_sec;
    int nsec = a.tv_nsec - b.tv_nsec;
    *target += (long)sec * 1000000000 + nsec;
}

void sequential_read_file(char* filename, int* nums, int* count)
{
    if (world_rank == 0) {
	    tmp = (int*) malloc(sizeof(int) * file_size);
        FILE* fp = fopen(filename, "rb");
        timeit(&s);
        fread(tmp, sizeof(int), file_size, fp);
        timeit(&e); calc_time(&io_time, e, s);
	    fclose(fp);
    }
    timeit(&s);
    MPI_Scatter(tmp, subset_size, MPI_INT, nums, subset_size, MPI_INT,
    			0, MPI_COMM_WORLD);
    timeit(&e); calc_time(&comm_time, e, s);
    *count = subset_size;
    free(tmp);
}

void sequential_write_file(char* filename, int* nums, int* count)
{
	if (world_rank == 0) tmp = (int*) malloc(sizeof(int) * file_size);
    timeit(&s);
    MPI_Gather(nums, subset_size, MPI_INT, tmp, subset_size, MPI_INT, 0, MPI_COMM_WORLD);
    timeit(&e); calc_time(&comm_time, e, s);
    if (world_rank == 0) {
        FILE* fp = fopen(filename, "wb");
        timeit(&s);
        fwrite(tmp, sizeof(int), file_size, fp);
        timeit(&e); calc_time(&io_time, e, s);
        fclose(fp);
        free(tmp);
    }
}
void mpi_read_file(char* filename, int* nums, int* count)
{
    MPI_File input;
    MPI_Status status;
    MPI_File_open(MPI_COMM_WORLD, filename,
                  MPI_MODE_RDONLY, MPI_INFO_NULL, &input);
    MPI_File_set_view(input, sizeof(int) * subset_size * world_rank, MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
    timeit(&s);
    MPI_File_read_all(input, nums, subset_size, MPI_INT, &status);
    timeit(&e); calc_time(&io_time, e, s);
    MPI_File_close(&input);
    MPI_Get_count(&status, MPI_INT, count);
    printf("#%d %d\n", world_rank, *count);
}

void mpi_write_file(char* filename, int* nums, int* count)
{
    MPI_File fh;
    MPI_Status status;
    MPI_File_open(MPI_COMM_WORLD, filename,
                  MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL, &fh);
    MPI_File_set_view(fh, sizeof(int) * subset_size * world_rank,
                      MPI_INT, MPI_INT, "native", MPI_INFO_NULL);
    timeit(&s);
    MPI_File_write_all(fh, nums, *count, MPI_INT, &status);
    timeit(&e); calc_time(&io_time, e, s);
    MPI_File_close(&fh);
}

int main(int argc, char** argv)
{
    int count;
    sscanf(argv[1], "%d", &file_size);

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    /** For data size is less then processors **/
    if (file_size < world_size) world_size = file_size;

#ifdef _TIME
    printf("world_size %d\n", world_size);
#endif
    MPI_Barrier(MPI_COMM_WORLD);
    subset_size = file_size / world_size;
    if (file_size % world_size) subset_size += 1;

    nums = (int*) malloc(subset_size * sizeof(int));

#ifndef _SEQ_IO
    sequential_read_file(argv[2], nums, &count);
    sequential_write_file(argv[3], nums, &count);
#else
    mpi_read_file(argv[2], nums, &count);
    mpi_write_file(argv[3], nums, &count);
#endif

    MPI_Barrier(MPI_COMM_WORLD);
    free(nums);
#ifdef _TIME
    printf("#Rank%d: total=%ld nsec (io=%ld, comm=%ld, comp=%ld)\n",
           world_rank,
           io_time + comm_time + comp_time,
           io_time, comm_time, comp_time);
#endif
	return 0;
}

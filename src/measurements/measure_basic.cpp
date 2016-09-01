#include <cstdio>
#include <ctime>
#include <algorithm>
#define timeit(t)   clock_gettime(CLOCK_REALTIME, t)
using namespace std;

int file_size, *a;
long io_time = 0, compute_time = 0, commute_time = 0;
struct timespec s, e;

void calc_time(long* target, struct timespec a, struct timespec b)
{
    int sec = a.tv_sec - b.tv_sec;
    int nsec = a.tv_nsec - b.tv_nsec;
    *target += ((long) sec) * 1000000000 + nsec;
}

int cmp(const void* a, const void* b)
{
    /* watch out! it may overflow with return x - y */
    int x = *(int*)a, y = *(int*)b;
    return (x > y) - (x < y);
}

int main(int argc, char const *argv[])
{
    sscanf(argv[1], "%d", &file_size);
    FILE* fp = fopen(argv[2], "rb");
    a = (int*) malloc(sizeof(int) * file_size);

    timeit(&s);
    fread(a, sizeof(int), file_size, fp);
    timeit(&e); calc_time(&io_time, e, s);
    fclose(fp);

    timeit(&s);
#ifdef CPP_SORT
    sort(a, a + file_size);
#else
    qsort(a, file_size, sizeof(int), cmp);
#endif
    timeit(&e); calc_time(&compute_time, e, s);

    FILE* fh = fopen(argv[3], "wb");
    timeit(&s);
    fwrite(a, sizeof(int), file_size, fh);
    timeit(&e); calc_time(&io_time, e, s);
    fclose(fh);

    printf("%ld %ld\n", io_time, compute_time);
    return 0;
}

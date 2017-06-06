/* resizable array -- LarryRuane@gmail.com -- 5-Jun-2017 */

#include <stdio.h>
#include <stdlib.h> /* for malloc, random */
#include <time.h>   /* for clock() */
#include <assert.h>
#include "rsarray.h"

/* NB if using this code to do performance measurement or comparison,
 * be sure to specify at least -O1 on the compile command line,
 * and also specify -DNDEBUG (to eliminate asserts)
 */

/* return a random integer between 0 and limit-1 */
static inline uint_t
rand_range(uint_t limit)
{
    assert(limit);
    return (uint_t)(random() % limit);
}

/************** simple array of integers */

#define RSA_ITEM_T int
#define RSA_CLASS_NAME myint

#include "rsarray_template.h"

#undef RSA_ITEM_T
#undef RSA_CLASS_NAME

static void
test_rsarray_basic(void)
{
    fprintf(stderr, "%s\n", __func__);
    myint_rsa_t my = myint_rsa_init();

    /* no-op (already zero length) */
    myint_rsa_realloc(&my, 0);

    /* make array one element in length */
    myint_rsa_realloc(&my, 1);

    /* set and read back the first (only) array element */
    *myint_rsa(&my, 0) = 77;
    assert(*myint_rsa(&my, 0) == 77);

    /* grow the array, zero the new part */
    myint_rsa_realloc(&my, 99);
    myint_rsa_zero(&my, 1, 98);
    assert(*myint_rsa(&my, 57) == 0);
    *myint_rsa(&my, 98) = 23;
    assert(*myint_rsa(&my, 98) == 23);

    /* append to end of array */
    *myint_rsa_append(&my) = 8;
    assert(myint_rsa_len(&my) == 100);
    *myint_rsa_append(&my) = 9;
    assert(myint_rsa_len(&my) == 101);
    assert(*myint_rsa(&my, 99) == 8);
    assert(*myint_rsa(&my, 100) == 9);

    /* clean up (free memory); this just sets length to zero */
    myint_rsa_free(&my);
    assert(myint_rsa_len(&my) == 0);
}

/* this allocates about 16g of memory */
static void
test_rsarray_large(void)
{
    fprintf(stderr, "%s\n", __func__);
    myint_rsa_t my = myint_rsa_init();

    /* largest possible 32-bit array, use last element  */
    myint_rsa_realloc(&my, 0xffffffff);
    *myint_rsa(&my, 0xfffffffe) = 23;
    assert(*myint_rsa(&my, 0xfffffffe) == 23);

    /* test large arrays for the 64-bit index version; the casts
     * are actually so the 32-bit index version compiles
     */
    if (sizeof(rsa_len_t) == 8) {
        /* 64-bit model, one element beyond 32-bit limit */
        myint_rsa_realloc(&my, (rsa_len_t)0x100000001);
        *myint_rsa(&my, (rsa_len_t)0x100000000) = 23;
        assert(*myint_rsa(&my, (rsa_len_t)0x100000000) == 23);

        /* a little larger (can't test max) */
        myint_rsa_realloc(&my, (rsa_len_t)0x100300001);
        *myint_rsa(&my, (rsa_len_t)0x1002f9fff) = 43;
        assert(*myint_rsa(&my, (rsa_len_t)0x1002f9fff) == 43);
    }
}

/* randomly read (and verify), write and resize an array,
 * can use up to 16g memory
 */
static
void test_rsarray_rand(void)
{
    fprintf(stderr, "%s\n", __func__);
    int *arr = malloc(0);
    rsa_len_t arr_len = 0;
    myint_rsa_t my = myint_rsa_init();

    uint_t i;
    for (i=0;i<40000;i++) {
        if (!i || !rand_range(2000)) {
            /* verify all before resize */
            rsa_len_t k;
            for (k=0;k<arr_len;k++) {
                assert(*myint_rsa(&my, k) == arr[k]);
            }

            rsa_len_t new_len = rand_range(1 << rand_range(32))+1;
            myint_rsa_realloc(&my, new_len);
            arr = realloc(arr, sizeof(int)*new_len);
            if (new_len > arr_len) {
                /* new elements should be zero */
                memset(&arr[arr_len], 0, sizeof(int)*(new_len-arr_len));
                myint_rsa_zero(&my, arr_len, new_len-arr_len);
            }
            arr_len = new_len;
        }
        assert(myint_rsa_len(&my) == arr_len);

        rsa_len_t const j = rand_range(arr_len);
        rsa_len_t k;
        if (rand_range(10)) {
            /* write the same random value to both places */
            *myint_rsa(&my, j) = i;
            arr[j] = i;
        } else {
            /* probably some interesting boundaries occur beyond 256 */
            int copy_arr[300];
            rsa_len_t len = sizeof(copy_arr) / sizeof(copy_arr[0]);
            if (len > arr_len - j) {
                len = arr_len - j;
            }
            if (rand_range(2)) {
                /* from the rsarray out to a normal array */
                myint_rsa_copyout(&my, j, copy_arr, len);
                for (k=0;k<len;k++) {
                    assert(copy_arr[k] == arr[j+k]);
                }
            } else {
                /* copy from a normal array into the rsarray */
                for (k=0;k<len;k++) {
                    copy_arr[k] = i+k;
                    arr[j+k] = i+k;
                }
                myint_rsa_copyin(&my, j, copy_arr, len);
            }
        }

        /* read and verify a random element */
        k = rand_range(arr_len);
        assert(*myint_rsa(&my, k) == arr[k]);

        if (0) {
            /* if a failure occurs, rerun the seed with this code enabled, to
             * to catch the bug as early as possible (assuming reads don't
             * affect the state)
             */
            for (k=0;k<arr_len;k++) {
                assert(*myint_rsa(&my, k) == arr[k]);
            }
        }
    }

    /* final check */
    rsa_len_t k;
    for (k=0;k<arr_len;k++) {
        assert(*myint_rsa(&my, k) == arr[k]);
    }
    myint_rsa_free(&my);
    free(arr);
}

/* relative performance depends on array length; if very large,
 * frequent cache misses causes RSA to perform worse relative
 * to standard array, more so than if array length is smaller
 */
#define PERF_LEN (32*1024)

static void
test_rsarray_perf_rsa(void)
{
    fprintf(stderr, "%s\n", __func__);
    myint_rsa_t my = myint_rsa_init();

    /* largest possible 32-bit array, use last element  */
    myint_rsa_realloc(&my, PERF_LEN);
    myint_rsa_zero(&my, 0, myint_rsa_len(&my));

    clock_t cstart = clock();
    rsa_len_t j;
    uint_t i;
    for (i=0;i<200000000;i++) {
        j = rand_range(myint_rsa_len(&my));
        /* write (anything) */
        *myint_rsa(&my, j) = i;
    }
    clock_t cend = clock();
    fprintf(stderr, "  cputime = %lu.%03lu sec\n",
            (cend - cstart)/CLOCKS_PER_SEC,
            ((cend - cstart) % CLOCKS_PER_SEC)/1000);
}

static void
test_rsarray_perf_arr(void)
{
    fprintf(stderr, "%s\n", __func__);
    int *arr = calloc(PERF_LEN, sizeof(int));
    memset(arr, 0, sizeof(int) * PERF_LEN);

    clock_t cstart = clock();
    rsa_len_t j;
    uint_t i;
    for (i=0;i<200000000;i++) {
        j = rand_range(PERF_LEN);
        /* write (anything) */
        arr[j] = i;
    }
    clock_t cend = clock();
    fprintf(stderr, "  cputime = %lu.%03lu sec\n",
            (cend - cstart)/CLOCKS_PER_SEC,
            ((cend - cstart) % CLOCKS_PER_SEC)/1000);
}

static void
test_rsarray_perf_rsa_seq(void)
{
    fprintf(stderr, "%s\n", __func__);
    myint_rsa_t my = myint_rsa_init();

    /* largest possible 32-bit array, use last element  */
    myint_rsa_realloc(&my, PERF_LEN);
    myint_rsa_zero(&my, 0, myint_rsa_len(&my));

    clock_t cstart = clock();
    rsa_len_t j = 0;
    uint_t i;
    for (i=0;i<200000000;i++) {
        //j = rand_range(myint_rsa_len(&my));
        /* write (anything) */
        *myint_rsa(&my, j) = i;
        if (++j >= PERF_LEN) j = 0;
    }
    clock_t cend = clock();
    fprintf(stderr, "  cputime = %lu.%03lu sec\n",
            (cend - cstart)/CLOCKS_PER_SEC,
            ((cend - cstart) % CLOCKS_PER_SEC)/1000);
}

static void
test_rsarray_perf_arr_seq(void)
{
    fprintf(stderr, "%s\n", __func__);
    int *arr = calloc(PERF_LEN, sizeof(int));
    memset(arr, 0, sizeof(int) * PERF_LEN);

    clock_t cstart = clock();
    rsa_len_t j = 0;
    uint_t i;
    for (i=0;i<200000000;i++) {
        //j = rand_range(myint_rsa_len(&my));
        /* write (anything) */
        arr[j] = i;
        if (++j >= PERF_LEN) j = 0;
    }
    clock_t cend = clock();
    fprintf(stderr, "  cputime = %lu.%03lu sec\n",
            (cend - cstart)/CLOCKS_PER_SEC,
            ((cend - cstart) % CLOCKS_PER_SEC)/1000);
}

int
main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s seed\n", argv[0]);
        return 1;
    }
    srandom(atoi(argv[1]));

    /* change the looping constants (runtimes), enable as you wish */
    if(1) test_rsarray_basic();
    if(1) test_rsarray_large();
    if(1) test_rsarray_rand();
    if(1) test_rsarray_perf_rsa();
    if(1) test_rsarray_perf_arr();
    if(1) test_rsarray_perf_rsa_seq();
    if(1) test_rsarray_perf_arr_seq();

    return 0;
}

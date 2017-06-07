/* resizable array -- LarryRuane@gmail.com -- 5-Jun-2017 */

#include <stdio.h>
#include <stdlib.h> /* for malloc, realloc, free */
#include <assert.h>
#include "rsarray.h"

/* this could be in a more public place */
#define assert_static(cond) (void) sizeof(char[(cond) ? 1 : -1])

static inline
rsa_len_t rsa_len_min(
        rsa_len_t v0,
        rsa_len_t v1) {
    return v0 < v1 ? v0 : v1;
}

static inline
rsa_len_t rsa_len_max(
        rsa_len_t v0,
        rsa_len_t v1) {
    return v0 > v1 ? v0 : v1;
}

static inline
unsigned long int rsa_len_howmany(
        rsa_len_t v,
        rsa_len_t d) {
    return ((unsigned long int)v + (d - 1)) / d;
}

static inline
unsigned long int rsa_len_roundup(
        rsa_len_t v,
        rsa_len_t d) {
    return rsa_len_howmany(v, d) * d;
}

/* it would be easy to code this without the builtin, but it's fast */
static inline
rsa_len_t roundup_pow2(
        rsa_len_t v
) {
    assert_static(sizeof(rsa_len_t) <= 8) ;

    /* special cases, these are already powers of 2 */
    if (v <= 2) return v ;

    /* clz ("count leading zeros") returns the number of contiguous
     * high bits of zero in its argument
     */
    int clz = __builtin_clzll(v-1) ;
    assert(clz > 0) ;
    return ((rsa_len_t)1) << ((8*sizeof(rsa_len_t)) - clz) ;
}

/* possibly modify the pointer p points at */
void rsarray_realloc(
        unsigned int const element_len_arg,
        unsigned int const const level,
        rsa_len_t old_len,
        rsa_len_t new_len,
        void *** const p)
{
    assert(level <= 3);
    assert(p);
    if (old_len == new_len) {
        /* nothing to do */
        return;
    }

    /* the max length that each of our (up to 64k) entries corresponds to,
     * example:
     * level 0: 0x0001.0000.0000.0000
     * level 1: 0x0000.0001.0000.0000
     */
    uint_t shift = rsa_shift() * (3-level);
    rsa_len_t const entry_len = ((rsa_len_t)1) << shift;

    /* calculate how many elements we need at this level,
     * for example, if level is 0 and len = 0x0003.0000.0000.00207
     * then nelements = 4 (3 plus a little rounded up, must be <= 64k)
     * buf if len = 0x0003.0000.0000.00000, then nelements = 3
     */
    uint_t const old_nelements =
        rsa_len_roundup(old_len, entry_len) >> shift;
    uint_t const new_nelements =
        rsa_len_roundup(new_len, entry_len) >> shift;

    /* allocate power of 2 number of elements, so round up to that */
    uint_t const element_len = level < 3 ? sizeof(void *) : element_len_arg;
    uint_t const old_alloc_len = roundup_pow2(old_nelements)*element_len;
    uint_t const new_alloc_len = roundup_pow2(new_nelements)*element_len;

    if (new_alloc_len > old_alloc_len) {
        if (old_alloc_len) {
            *p = realloc(*p, new_alloc_len); /* from old_alloc_len */
        } else {
            *p = malloc(new_alloc_len);
        }
    }

    if (level < 3) {
        uint_t max_nelements = uint_max(old_nelements, new_nelements);
        assert(max_nelements <= (1 << rsa_shift()));
        uint_t ofs;
        for (ofs=0;ofs<max_nelements;ofs++) {
            rsa_len_t old_sub_len = rsa_len_min(old_len, entry_len);
            rsa_len_t new_sub_len = rsa_len_min(new_len, entry_len);
            rsarray_realloc(element_len_arg, level+1,
                    old_sub_len, new_sub_len,
                    (void ***)(&(*p)[ofs]));
            assert(old_len >= old_sub_len);
            old_len -= old_sub_len;
            assert(new_len >= new_sub_len);
            new_len -= new_sub_len;
        }
        assert(!old_len);
        assert(!new_len);
    }

    if (new_alloc_len < old_alloc_len) {
        if (new_alloc_len) {
            *p = realloc(*p, new_alloc_len); /* from old_alloc_len */
        } else {
            free(*p); /* old_alloc_len */
        }
    }
}

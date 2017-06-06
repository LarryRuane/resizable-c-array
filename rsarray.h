/* resizable array -- LarryRuane@gmail.com -- 5-Jun-2017 */

/* length of array (index and number of elements), should be 32 or 64 bit
 *
 * note that 64-bit array length (indices) can be implemented
 * within the 32-bit model (32 bit pointers).
 *
 * we assume 8-bit bytes
 */
typedef unsigned int rsa_len_t;         /* 32-bit indices */
//typedef unsigned long int rsa_len_t;  /* 64-bit indices */

typedef unsigned int uint_t;

static inline
uint_t uint_min(
        uint_t const v0,
        uint_t const v1) {
    return v0 < v1 ? v0 : v1;
}

static inline
uint_t uint_max(
        uint_t const v0,
        uint_t const v1) {
    return v0 > v1 ? v0 : v1;
}

/**** internal use (by rsarray_template.h) only: */

static inline
uint_t rsa_shift(void) {
    /* 8 bits per byte; 4 levels */
    return 8*sizeof(rsa_len_t)/4;
}

static inline
uint_t rsa_len(void) {
    return ((rsa_len_t)1) << rsa_shift();
}

static inline
uint_t rsa_mask(void) {
    return rsa_len() - 1;
}
/**** (end internal (by rsarray_template.h) only) */

/* this is the only non-inline function in rsarray */
void rsarray_realloc(
        uint_t element_len_arg,
        uint_t const level,
        rsa_len_t old_len,
        rsa_len_t new_len,
        void *** const p);

/* resizable array -- LarryRuane@gmail.com -- 5-Jun-2017 */

#include <string.h> /* memset, memcpy */

#define RSA_METHOD_HELP2(class,func) class ## _rsa_ ## func
#define RSA_METHOD_HELP(class,func) RSA_METHOD_HELP2(class,func)
#define RSA_METHOD(func) RSA_METHOD_HELP(RSA_CLASS_NAME,func)

#define RSA_ACCESS_HELP2(class) class ## _rsa
#define RSA_ACCESS_HELP(class) RSA_ACCESS_HELP2(class)
#define RSA_ACCESS RSA_ACCESS_HELP(RSA_CLASS_NAME)

#define RSA_TYPE_HELP2(class) class ## _rsa_t
#define RSA_TYPE_HELP(class) RSA_TYPE_HELP2(class)
#define RSA_T RSA_TYPE_HELP(RSA_CLASS_NAME)

typedef struct {
    rsa_len_t len;    /* length (# elements) in the array */
    void ** root;
} RSA_T;

static __attribute__ ((unused))
RSA_T RSA_METHOD(init)(void) {
    /* compiler initializes to zeros */
    static RSA_T const init;
    return init;
}

/* always four levels so code is straight-line non-conditional (fast) */
static inline __attribute__ ((unused))
RSA_ITEM_T * RSA_ACCESS(
        RSA_T const * s,
        rsa_len_t ofs)
{
    assert(ofs < s->len);
    assert(s->root);
    void ** p1 = s->root[ofs >> (3*rsa_shift())];
    assert(p1);
    void ** p2 = p1[(ofs >> (2*rsa_shift())) & rsa_mask()];
    assert(p2);
    /* the lowest level contains the user's items */
    RSA_ITEM_T * p3 = p2[(ofs >> (1*rsa_shift())) & rsa_mask()];
    assert(p3);
    return &p3[ofs & rsa_mask()];
}

static inline __attribute__ ((unused))
void RSA_METHOD(realloc)(
        RSA_T * s,
        rsa_len_t len)
{
    rsarray_realloc(sizeof(RSA_ITEM_T), 0, s->len, len, &s->root);
    s->len = len;
}

/* free a rsarray by setting its length to zero */
static __attribute__ ((unused))
void RSA_METHOD(free)(
        RSA_T * s)
{
    rsarray_realloc(sizeof(RSA_ITEM_T), 0, s->len, 0, &s->root);
    s->len = 0;
}

static inline __attribute__ ((unused))
rsa_len_t RSA_METHOD(len)(
        RSA_T const * s)
{
    return s->len;
}

/* extend array length by 1, return address of (new) last element */
static __attribute__ ((unused))
RSA_ITEM_T * RSA_METHOD(append)(
        RSA_T * s)
{
    uint_t const new_ofs = s->len;
    RSA_METHOD(realloc)(s, s->len+1);
    return RSA_ACCESS(s, new_ofs);
}

/* import from a normal array into the rsarray */
static __attribute__ ((unused))
void RSA_METHOD(zero)(
        RSA_T const * const s,
        uint_t ofs,
        uint_t len)

{
    assert(ofs + len <= s->len);
    uint_t src_ofs = 0;
    while (len) {
        uint_t const i = uint_min(rsa_len() - (ofs & rsa_mask()), len);
        memset(RSA_ACCESS(s, ofs), 0, i * sizeof(RSA_ITEM_T));
        src_ofs += i;
        ofs += i;
        len -= i;
    }
}

/* import from a normal array into the rsarray */
static __attribute__ ((unused))
void RSA_METHOD(copyin)(
        RSA_T const * const s,
        uint_t ofs,
        RSA_ITEM_T const * const p,
        uint_t len)
{
    assert(ofs + len <= s->len);
    uint_t src_ofs = 0;
    while (len) {
        uint_t const i = uint_min(rsa_len() - (ofs & rsa_mask()), len);
        memcpy(RSA_ACCESS(s, ofs), &p[src_ofs], i * sizeof(RSA_ITEM_T));
        src_ofs += i;
        ofs += i;
        len -= i;
    }
}

/* export from the rsarray to a normal array */
static __attribute__ ((unused))
void RSA_METHOD(copyout)(
        RSA_T const * const s,
        uint_t ofs,
        RSA_ITEM_T * const p,
        uint_t len)
{
    assert(ofs + len <= s->len);
    uint_t src_ofs = 0;
    while (len) {
        uint_t const i = uint_min(rsa_len() - (ofs & rsa_mask()), len);
        memcpy(&p[src_ofs], RSA_ACCESS(s, ofs), i * sizeof(RSA_ITEM_T));
        src_ofs += i;
        ofs += i;
        len -= i;
    }
}

#undef RSA_METHOD_HELP2
#undef RSA_METHOD_HELP
#undef RSA_METHOD

#undef RSA_ACCESS_HELP2
#undef RSA_ACCESS_HELP
#undef RSA_ACCESS

#undef RSA_TYPE_HELP2
#undef RSA_TYPE_HELP
#undef RSA_T

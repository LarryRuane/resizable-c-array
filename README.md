# resizable-c-array
(C language) memory- and cpu-efficient data structure with C array semantics except resizing (grow or shrink) is in-place and cheap.

This code (tiny library, about 300 lines) implements a dynamically-allocated data structure that can be treated the same as a standard dynamically-allocated C array, except that pointer arithmetic is not allowed. (More on this below.) The main advantage of this data structure is that the array can grow or shrink at very low cost, in-place, in small constant time, regardless of the size of the array. In contrast, one grows a standard C array by allocating a new array of the required length, copying the old array to the (first part of the) new array, and freeing the old array. This causes a memory usage spike that can, in some situations, run the system completely out of memory. It's also slow, as every element needs to be copied (although `memcpy` can help).

This library uses the standard C library memory allocation functions, `malloc`, `realloc`, and `free`. A secondary advantage in some environments is that the library allocates memory in "reasonable" size chunks (most commonly, 2k bytes). In some systems, once a memory buffer of a particular length is allocated it remains that length from then on. That is, if it is freed, then only a request of that length (or often, within a power of 2 of that length) will cause the buffer to be reused. In general, memory remains less fragmented if all or most of the allocations are small and of the same size.

I've developed and tested this code only using gcc (on Linux). Porting it elsewhere will likely require some tweaks.

# user interface
This library is written in a style that is unusual for C, but will be familiar to C++ programmers. It is analogous to a template container library, such that before creating an instance of an array, you indicate the type of its elements, and code is then "generated" for that particular type. For example, you might create an array type to contain 16-bit (short) integers. Then you may create any number of instances of this array type. Or you may create an array of some arbitrarily complex structure. Or an array of characters, or an array of pointers to a particular type.

All accesses of these arrays are fully type-checked.

It's easiest to see with an example (see the source file rsarray_test.c for this and other examples). The first step is to define an array type for a given base type:
```
#include "rsarray.h"

/************** simple array of integers */
#define RSA_ITEM_T int
#define RSA_CLASS_NAME myint

#include "rsarray_template.h"

#undef RSA_ITEM_T
#undef RSA_CLASS_NAME
```
This defines an array-type to hold integers (type `int`). The type that RSA_ITEM_T is defined as can be any type, built-in, user-supplied, pointer and so on. You also specify an arbitrary class name, in this case `myint`. By next including `rsarray_template.h`, code is generated at that point in the source file. It's always a good idea to then undefine the two symbols, in case an array of a different base type is defined within the same source file.

Next we'll see a function that creates an instance of this array:
```
static void
test_rsarray_basic(void)
{
    myint_rsa_t my = myint_rsa_init();
```
Every array has an initial length of zero. The `my` variable is not a pointer to allocated memory; it does not need to be freed. It is actually a tiny structure; setting its value to the return of the init function simply zeros it (you can do that yourself in a different way). The function could return at this point because the array is zero-length (no allocated memory).

Before we can add a value to this array, we need to give it non-zero length. We'll make it one entry long. This does allocate memory (using `malloc`).
```
    /* make array one element in length */
    myint_rsa_realloc(&my, 1);
```
A disadvantage of this library is that a special syntax is needed to access the array. Rather than having separate `get` (or `read`) and `set` (or `write`) functions, I decided to implement a single "access" function that returns the address of the array element. There will usually be an asterisk before calls to this function. When the expression `*myint_rsa(..)` is on the left of the assignment, the value is being set; otherwise, when it is part of an expression, it is being read. Here we see both:
```
    /* set and read back the first (only) array element */
    *myint_rsa(&my, 0) = 77;
    assert(*myint_rsa(&my, 0) == 77);
```
The following code shows the main benefit of this library. First, create an array with 1 billion elements. Then grow it by one element, which takes almost no time and no additional temporary space.
```
    myint_rsa_realloc(&my, 1000000000);
    myint_rsa_realloc(&my, 1000000001);
```
You can also shrink the array, and exactly the appropriate amount of memory is released back to the system (in this case, 1b minus 5k). The elements at indices 5000 and beyond are discarded:
```
    myint_rsa_realloc(&my, 5000);
```
When an array is no longer needed (and certainly before it goes out of scope), its length must be set to zero. There is a shorthand for that; the following two are equivalent:
```
    myint_rsa_realloc(&my, 0);
    myint_rsa_free(&my);
```
(There is no harm in setting the length to zero, or anything else, multiple times.)

# pointer arithmetic
As mentioned, pointer arithmetic is not allowed with these arrays. Pointer arithmetic includes subtracting the base of an array from a pointer into the array to generate an index, for example:

```
  int array[10];
  int *p = &array[5];
  printf("%u", p - array);      /* this prints 5 */
  printf("%u", p - &array[1]);  /* this prints 4 */
```

Another example is adding integer to a pointer into the array to generate another pointer:
```
  int array[10];
  int *p = &array[5];
  int *q = &p[3];               /* q is &array[8] */
  int *q = p + 3;               /* q is &array[8] */
```
Here's another common type of pointer arithmetic:
```
  int array[10];
  int *p;
  for (p = array; p < &array[10]; p++) {
      ...
  }
```
None of these is allowed; the only way to access the array is through indexing (rewriting the last example):
```
  int array[10];
  int *i;
  for (i = 0; i < 10; i++) {
      int *p = &array[i];
      ...
  }
```

# 32 or 64 bit index
This library can operate with either 32-bit or 64-bit indices (default 32-bit). The 32-bit version supports arrays with up to 4g entries. The 64-bit version supports much larger arrays. The symbol `rsa_len_t` should be defined to be either a 32-bit types (`unsigned int`) or 64-bit type (`unsigned long int` with gcc at least).

# implementation
An rsarray (resizable array) is a four-level tree structure. In the 32-bit version, each level corresponds to 8 bits of the index. Each level has a fanout of 256. The first three levels are arrays of pointers to the next level. These arrays are (up to) 2k bytes in length (assuming 8-byte pointers, 256 times 8). The last level contains the actual user entries (as just a standard C array). The size in bytes of these arrays is (up to) 256 times the size of each array entry.

The reason there are always exactly four levels, each corresponding to exactly 8 bits, is so that the `access` method (in the example, `myint_rsa()`), which is critical to performance, has no conditionals and no loops. It is straight-line code with constant shift and mask arguments, which can be highly optimized by modern instruction pipeline-aware compilers. See the `ACCESS` method in rsarray_template.h. (The functions called there, `rsa_shift` and `rsa_mask`, are trivial and inline, returning constants.)

All of these arrays (of either pointers or user elements) are actually allocated as powers of 2. This provides a balance between unused memory and reallocations. As an example, when an array grows from 4 to 5 elements, the underlying array (with the user elements) is reallocated (`realloc`)to be large enough to hold 8 elements. Then the array can grow to 6, 7 and 8 with no calls to `realloc`. So reallocations do occur, but they're never for anything larger than 128 to 256 entries (again, whether these are 8-byte pointers or user elements).

Note that this design means that calling code should not cache pointers to elements across array reallocations.

# memory overhead
It is easy to calculate the memory overhead as a function of the base type. The highest overhead would be for an array of characters. If the string is significantly large, then for each 256 characters, there is one 8-byte pointer to it, which is an overhead of just over 3%. (There are arrays of pointers in higher levels, but their space usage is trivial.) There is actually some overhead for each `malloc` that we're neglecting here. If the array elements are 8-byte pointers, then the overhead is 8 / 2k which is about 0.4%. The overhead becomes lower as the element increases.

The memory overhead scales downward well too. An array with one element creates 4 allocations, 3 that are each 8 bytes (one pointer), and one that is the length of the base type.

# conclusion
I hope this is useful to you! If you end up using this code, I'd appreciate it if you let me know at LarryRuane@gmail.com.

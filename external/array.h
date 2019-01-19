/** Demizdors' internal array implmentation. */

#ifndef VEE_ARRAY_H
#define VEE_ARRAY_H

#include "util.h"

/** Generic array of type `T` */
#define Array(T) struct { \
	T* data; \
	size_t size; \
	size_t capacity; \
}


typedef Array(void) Array;
typedef size_t ArrayIt;

/** Initialize array `A` to hold at least `N` items. Set `N` to 0 to not allocate any memory yet.
 * Returns VEE_OK[0] on success. */
#define Array_create(A, N) ({ \
	int R__ = VEE_OK; \
	*((Array*)(A)) = (Array){0}; \
	R__ = Array_reserve((A), N); \
	R__; \
})

#define Array_destroy(A) ({ \
	if((A) != NULL) { \
		free((A)->data); *((Array*)(A)) = (Array){0}; \
	} \
})

extern int array_reserve__(Array*, size_t, size_t);
/** Ensures that the array `A` can hold at least `N` items.
 * Returns VEE_OK[0] on success. */
#define Array_reserve(A, N) ( array_reserve__((Array*)(A), sizeof(*(A)->data), N) )


/** Adds the value `V` at the end of the array `A`. 
 * Returns VEE_OK[0] on success. */
#define Array_append(A, V) ({ \
	ArrayIt I__ = Array_size(A); \
	int R__ = Array_reserve((A), Array_size(A) + 1); \
	if(R__ == VEE_OK) { (Array_data(A))[I__] = (V); (A)->size += 1; } \
	R__; \
})

/** Adds the value `V` to the start of the array `A`. This can be very slow on large 
 * arrays because all the items must be shifted by one.
 * Returns VEE_OK[0] on success. */
#define Array_prepend(A, V) ({ \
	int R__ = array_insert__((Array*)(A), 0, sizeof(*(A)->data), 1); \
	if(R__ == VEE_OK) Array_data(A)[0] = V; \
	R__; \
})

/** Adds the value `V` to the end of array `A`. 
 * Returns VEE_OK[0] on success. */
#define Array_push(A, V) (Array_append(A, V))


/** Removes value at the end of array `A`. */
#define Array_pop(A) ({ \
	if(Array_size(A) != 0) (A)->size -= 1; \
})


extern int array_insert__(Array*, ArrayIt, size_t, size_t);
/** Inserts `N` items from `V` before position `P` into array `A`. `V` shouldn't be part of `A`.
 * Returns VEE_OK[0] on success. */
#define Array_insert(A, P, V, N) ({ \
	int R__ = array_insert__((Array*)(A), P, sizeof(*(A)->data), N); \
	if(R__ == VEE_OK) memmove(&Array_data(A)[P], V, N*sizeof(*(A)->data)); \
	R__; \
})

extern size_t array_remove__(Array* , ArrayIt, size_t, size_t);
/** Remove `N` items from array `A` at position `P`. Returns number of items 
 * succesfully removed (might be smaller than `N`). */
#define Array_remove(A, P, N) (array_remove__((Array*)(A), P, sizeof(*(A)->data), N))

/** Compact the array `A` so that its size and capacity become the same. This is usefull to 
 * conserve memory when the array is not expected to grow anymore. */
#define Array_compact(A) ({ \
	if((A)->capacity != 0 && (A)->size != 0) { \
		void* T__ = realloc((A)->data, (A)->size*sizeof(*(A)->data)); \
		if(T__ != NULL) { \
			(A)->data = T__; (A)->capacity = (A)->size; \
		} \
	} \
})

/** Returns how many items are in array. */
#define Array_size(A) ({ \
	size_t R__ = (A)->size; \
	R__; \
})

/** Returns maximum number of items the array can hold before it gets resized. */
#define Array_capacity(A) ({ \
	size_t R__ = (A)->capacity; \
	R__; \
})

/** Returns a pointer to the internal memory. */
#define Array_data(A)	((A)->data)


#define Array_at(A, I) ((A)->data[I])


#endif
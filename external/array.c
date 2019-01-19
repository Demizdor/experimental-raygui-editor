/** Demizdors' internal array implmentation. */

#include "array.h"

#include <string.h> //for memmove()
#include <stdlib.h> //for realloc()/calloc()

int array_reserve__(Array* a, size_t t, size_t n) {
	if(a == NULL) return VEE_BAD_ARG;
	if(n > a->capacity) {
		n = roundup(n);
		void* data = (a->data != NULL)?realloc(a->data, t*n):calloc(n, t);
		if(data == NULL) return VEE_OUT_OF_MEMORY;
		a->data = data;
		a->capacity = n;
	}
	return VEE_OK;
}

int array_insert__(Array* a, ArrayIt i, size_t t, size_t n) {
	if(n == 0 || a == NULL) return VEE_BAD_ARG;
	
	// i == a->size is allowed and not out of bounds because we always insert at least one element
	if(i > a->size) return VEE_OUT_OF_BOUNDS; 
	
	const size_t nsz = a->size + n;
	int r = array_reserve__(a, t, nsz);
	if(r == VEE_OK) {
		size_t m = a->size - i;
		if(m != 0) { 
			// shift `m` old entries to the right to make room for `n` new entries
			void* src = a->data + i*t;
			void* dest = src + n*t;
			memmove(dest, src, m*t);
		}
		a->size = nsz;
	}
	return r;
}


size_t array_remove__(Array* a , ArrayIt p, size_t t, size_t n) {
	if(a == NULL || n == 0 || p >= a->size) return 0;
	
	//how many items can we really remove to not go out of bounds
	n = p+n>a->size?a->size-p:n;
	//how many items to shift
	size_t m = a->size - p - n;
	if(m != 0) {
		void* dest = a->data + p*t;
		void* src = dest + n*t;
		memmove(dest, src, m*t);
	}
	a->size -= n;
	return n;
	
}
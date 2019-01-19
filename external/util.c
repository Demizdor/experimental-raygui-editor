#include "util.h"

inline size_t roundup(size_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
	if(sizeof(size_t) == 8) v |= v >> 32;
    ++v;
    return v;
}

#ifdef __linux__
#include <execinfo.h>	//backtrace
#include <unistd.h>		//STDERR_FILENO
void print_backtrace() {
    void* array[256];
    size_t size = backtrace(array, 256);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
}
#else
//TODO: make this work on other OS (windows example here -> https://stackoverflow.com/questions/5693192/win32-backtrace-from-c-code)
void print_backtrace() { }
#endif

inline uint32_t fnv32_1a(char* p, size_t sz) {
    uint32_t hash = FNV32_OFFSET;
    for(const char* end = &p[sz]; end != p; ++p)
        hash = (*p ^ hash) * FNV32_PRIME;
    return hash;
}


inline uint64_t fnv64_1a(char* p, size_t sz) {
    uint64_t hash = FNV64_OFFSET;
    for(const char* end = &p[sz]; end != p; ++p)
        hash = (*p ^ hash) * FNV64_PRIME;
    return hash;
}

/*
// error reporting
#include <stdarg.h>
static __thread char error__[2048] = {0};
void set_error__(char* fmt, ...) {
	va_list args;
	va_start (args, fmt);
	
	vsnprintf(&error__[0], sizeof(error__)/sizeof(error__[0]), fmt, args)
		
	va_end(args);
}

char* get_error() {
	return (error__[0] == '\0'):NULL?&error__[0];
}

void clear_error() {
	error__ = {0};
}
 */


#ifndef VEE_UTIL_H
#define VEE_UTIL_H

#include <stdbool.h>    //for bool
#include <stdint.h>     //for (u)intXX_t types
#include <inttypes.h>   //for PRIu/iXX
#include <stdlib.h>     
#include <stdio.h>
#include <string.h>


// -------
// DEBUG
// -------

#ifdef __linux__
#define COLOR_RESET "\e[0m"
#define COLOR_BOLD "\e[1m"
#define COLOR_DEBUG "\e[2m"
#define COLOR_PANIC "\e[31m"
#define COLOR_WARN "\e[33m"
#else
#define COLOR_RESET ""
#define COLOR_BOLD ""
#define COLOR_DEBUG ""
#define COLOR_PANIC ""
#define COLOR_WARN ""
#endif

/* Write a debug message */
#ifdef NDEBUG
    #define debug(fmt, ...)
	#define debug_if(cond, fmt, ...)
#else
    #define debug(fmt, ...) ({ \
        fprintf(stdout, COLOR_DEBUG "# " fmt COLOR_RESET "\n", ##__VA_ARGS__); \
    })
	
	/* Write a debug message if condition is met. */
	#define debug_if(cond, fmt, ...) ({ \
		if(cond) { debug(fmt, ##__VA_ARGS__); } \
	})
#endif

/* Write a info message to stdout */
#define info(fmt, ...) ({ \
    fprintf(stdout, fmt"\n", ##__VA_ARGS__); \
})

/* Write a info message to stdout if condition is met. */
#define info_if(cond, fmt, ...) ({ \
	if(cond) { info(fmt, ##__VA_ARGS__); } \
})

/* Write a warning to stderr */
#define warn(fmt, ...)({ \
    fprintf(stderr, COLOR_BOLD COLOR_WARN"! " fmt COLOR_RESET"\n", ##__VA_ARGS__); \
})

/* Write a warning to stderr if condition is met. */
#define warn_if(cond, fmt, ...) ({ \
	if(cond) { warn(fmt, ##__VA_ARGS__); } \
})


#define panic(fmt, ...) ({ \
	fprintf(stderr, "\n" COLOR_PANIC "!! PANIC !!\n!! from %s() on line %i in `%s`\n!! reason: `" fmt "`\n!! -----\n", \
            __FUNCTION__, __LINE__, __FILE__, ##__VA_ARGS__); \
	print_backtrace(); \
	fprintf(stderr, "!! -----" COLOR_RESET "\n"); \
	fflush(stderr); \
	exit(EXIT_FAILURE); \
})

extern void print_backtrace();
/* If condition is met write a message to stderr and terminate the application. */
#define panic_if(cond, fmt, ...) ({ \
    if(cond) { panic(fmt, ##__VA_ARGS__); } \
})


// -------
// OTHER
// -------

/* Returns `v` rounded up to the nearest power of 2. */
extern size_t roundup(size_t v);

enum {
	VEE_OK = 0,
	VEE_OUT_OF_MEMORY = -1000,
	VEE_NOT_FOUND,
	VEE_OUT_OF_BOUNDS,
	VEE_BAD_ARG,
};

// Macros used when generating names for generic constructs
#define STRINGIFY(A) #A
#define STR(A) STRINGIFY(A)
#define GEN2__(A, B) A##_##B##_
#define GEN3__(A, B, C) A##_##B##_##C##_
#define GEN__(G, T) GEN2__(G, T)
#define FN__(G, T, N) GEN__(G, GEN2__(N, T))

#define FNV32_OFFSET    0x811c9dc5
#define FNV32_PRIME     0x1000193
#define FNV64_OFFSET    0xcbf29ce484222325
#define FNV64_PRIME     0x100000001b3

#define H0(c,h)     ((c^h)*FNV32_PRIME)
#define H1(s,p,h)   H0((uint8_t)s[(p)<strlen(s)?strlen(s)-1-(p):strlen(s)], ((p!=strlen(s)-1)?h:FNV32_OFFSET))
#define H4(s,p,h)   H1(s,p,H1(s,p+1,H1(s,p+2,H1(s,p+3,h))))
#define H16(s,p,h)  H4(s,p,H4(s,p+4,H4(s,p+8,H4(s,p+12,h))))
#define H64(s,p,h)  H16(s,p,H16(s,p+16,H16(s,p+32,H16(s,p+48,h))))
#define H256(s,p,h) H64(s,p,H64(s,p+64,H64(s,p+128,H64(s,p+192,h))))
/** Static fnv1a hash 32 bits */
#define FNV32(s)    ((strlen(s)==0)?FNV32_OFFSET:(uint32_t)H256(s,0,0))

#define HL0(c,h)     ((c^h)*FNV64_PRIME)
#define HL1(s,p,h)   HL0((uint8_t)s[(p)<strlen(s)?strlen(s)-1-(p):strlen(s)], ((p!=strlen(s)-1)?h:FNV64_OFFSET))
#define HL4(s,p,h)   HL1(s,p,HL1(s,p+1,HL1(s,p+2,HL1(s,p+3,h))))
#define HL16(s,p,h)  HL4(s,p,HL4(s,p+4,HL4(s,p+8,HL4(s,p+12,h))))
#define HL64(s,p,h)  HL16(s,p,HL16(s,p+16,HL16(s,p+32,HL16(s,p+48,h))))
#define HL256(s,p,h) HL64(s,p,HL64(s,p+64,HL64(s,p+128,HL64(s,p+192,h))))
/** Static fnv1a hash 64 bits */
#define FNV64(s) ( (strlen(s)==0)?FNV64_OFFSET:(uint64_t)HL256(s,0,0) )


extern uint32_t fnv32_1a(char*, size_t);
extern uint64_t fnv64_1a(char*, size_t);

/*
#define set_error(fmt, ...) set_error__(fmt " in " __FUNCTION__ ":"__LINE__" -> `"__FILE__"`" , ##__VA_ARGS__)

extern void set_error__(char* fmt, ...);
extern char* get_error(void);
extern void clear_error(void);
*/
#endif
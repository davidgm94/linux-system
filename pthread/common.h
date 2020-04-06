#include <stdint.h>
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/***
 * Macro to count the elements of an static array
 * ***/
#define COUNT_OF(__arr__) (((sizeof(__arr__)) / (sizeof(__arr__[0]))))

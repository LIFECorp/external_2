/*
 * harfbuzz-debug.h
 *
 */

#ifndef HB_DEBUG_H_
#define HB_DEBUG_H_
#ifdef __cplusplus
extern "C" {
#endif
#define ANDROID_DEBUG 0

#if ANDROID_DEBUG
#define HBDebug(...) Android_Debug(__FILE__, __LINE__, \
                                   __FUNCTION__, __VA_ARGS__)
#else
#define HBDebug
#endif

void Android_Debug(const char* file, int line, const char* function, const char* format, ...)
        __attribute__((format(printf, 4, 5)));  /* 4=format 5=params */;
#ifdef __cplusplus
}
#endif
#endif /* HARFBUZZ_DEBUG_H_ */

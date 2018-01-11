#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


#ifndef DEBUG_CTRL_H
#define DEBUG_CTRL_H

#define se uv_strerror

#define DEBUG 1
#define INFO  2
#define WARN  3
#define ERR   4
#define CRIT  5

extern int xxxDebugLevel __attribute__((unused));
extern FILE *xxxLog __attribute__((unused));

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define SET_DEBUG_LEVEL(x)  (xxxDebugLevel=(x))
#define GET_DEBUG_LEVEL()   (xxxDebugLevel)

#define slogf(inputLV,format,...)                               \
    do {                                                \
        time_t tt; \
        char bb[30]; \
        struct tm* tm_info; \
        time(&tt); \
        tm_info = localtime(&tt); \
        strftime(bb, 30, "%H:%M:%S", tm_info); \
        if(xxxLog!=NULL) {                              \
            fprintf(xxxLog,"%s %5s|%-8.8s:%4d %-14.14s # " format ,bb,#inputLV,__FILENAME__,__LINE__,__func__,##__VA_ARGS__);                                          \
        }                                               \
        if((inputLV)>=xxxDebugLevel) {                  \
            /*if(inputLV==INFO) fprintf(stderr,WHT);*/          \
            if(inputLV==WARN) fprintf(stderr,YEL);          \
            if(inputLV==ERR) fprintf(stderr,RED);           \
            if(inputLV==CRIT) fprintf(stderr,CYN);          \
            fprintf(stderr,"%s %d|%5s|%-8.8s:%4d %-14.14s # " format RESET,bb,getpid(),#inputLV,__FILENAME__,__LINE__,__func__,##__VA_ARGS__);                                     \
            if((inputLV)>=CRIT) { exit(__LINE__); } }       \
    } while(0)

void open_flog(const char *);

#endif /* HEADER END */




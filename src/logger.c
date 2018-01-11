#include <stdio.h>

int xxxDebugLevel = 1; /* 2 == INFO */
FILE* xxxLog = NULL;

void open_flog(const char *name)
{
    if(name)
        xxxLog = fopen(name, "a");
}


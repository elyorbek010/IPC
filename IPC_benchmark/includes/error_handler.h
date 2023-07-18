#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <stdio.h>

#define EXIT_ON_ERROR(val) \
    do                     \
    {                      \
        if (val != 0)      \
        {                  \
            perror(#val);  \
            exit(1);       \
        }                  \
    } while (0)

#define RETURN_ON_ERROR(val) \
    do                       \
    {                        \
        if (val != 0)        \
        {                    \
            perror(#val);    \
            return -1;       \
        }                    \
    } while (0)

#endif // ERROR_HANDLER_H
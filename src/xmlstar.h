#ifndef XMLSTAR_H
#define XMLSTAR_H

#include <config.h>
#include <stdlib.h>

typedef enum { /* EXIT_SUCCESS = 0, EXIT_FAILURE = 1, */
    EXIT_BAD_ARGS = EXIT_FAILURE+1, EXIT_BAD_FILE,
    EXIT_LIB_ERROR, EXIT_INTERNAL_ERROR } exit_status;

#define COUNT_OF(array) (sizeof(array)/sizeof(*array))
#endif  /* XMLSTAR_H */

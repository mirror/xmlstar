#ifndef XMLSTAR_H
#define XMLSTAR_H

#include <config.h>
#include <stdlib.h>

#include <libxml/xmlreader.h>

typedef enum { /* EXIT_SUCCESS = 0, EXIT_FAILURE = 1, */
    EXIT_BAD_ARGS = EXIT_FAILURE+1, EXIT_BAD_FILE,
    EXIT_LIB_ERROR, EXIT_INTERNAL_ERROR } exit_status;

#define COUNT_OF(array) (sizeof(array)/sizeof(*array))

typedef struct _errorInfo {
    const char *filename; /* file error occured in, if any, else NULL */
    xmlTextReaderPtr xmlReader;
    int verbose;
} ErrorInfo;

void reportError(void *ptr, xmlErrorPtr error);

int parseNSArr(xmlChar** ns_arr, int* plen, int argc, char **argv);
void cleanupNSArr(xmlChar **ns_arr);
extern xmlChar *ns_arr[];

#endif  /* XMLSTAR_H */

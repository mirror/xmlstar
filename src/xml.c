/* $Id: xml.c,v 1.6 2002/11/13 06:45:45 mgrouch Exp $ */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

void usage(int argc, char **argv)
{
    FILE* o = stderr;

    char *path = argv[0];
    char *dirc;
    char *cmd;
    
    dirc = strdup(path);
    cmd = basename(dirc);
    
    fprintf(o, "XMLStarlet command line XML toolkit (version 0.0.1, Nov 2002)\n");
    fprintf(o, "Usage: %s <command> [<options>]\n", cmd);
    fprintf(o, "where <command> is one of:\n");
    fprintf(o, "      ed   (or edit)      - Edit XML document\n");
    fprintf(o, "      sel  (or select)    - Select data or query XML document(s)\n");
    fprintf(o, "      tr   (or transform) - Transform XML document\n");
    fprintf(o, "      val  (or validate)  - Validate XML document(s)\n");
    fprintf(o, "Type: %s <command> --help <ENTER> for more options\n", cmd);

#if 0
    /* TODO */
    fprintf(o, "      diff                - Find differences between XML documents\n");
#endif

    if (dirc)
    {
        free(dirc);
        cmd = 0;
    }

    exit(1);
}  

int main(int argc, char **argv)
{
    usage(argc, argv);
    exit(0);
}

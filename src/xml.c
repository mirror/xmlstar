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
    
    fprintf(o, "Usage: %s <command> [<options>]\n", cmd);
    fprintf(o, "where <command> is one of:\n");
    fprintf(o, "      tr  (or transform) - Transform XML document\n");
    fprintf(o, "      ed  (or edit)      - Edit XML document\n");
    fprintf(o, "      sel (or select)    - Select data or query XML document(s)\n");
    fprintf(o, "      val (or validate)  - Validate XML document(s)\n");
    fprintf(o, "Type: %s <command> --help <ENTER> for more options\n", cmd);

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

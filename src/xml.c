/* $Id: xml.c,v 1.8 2002/11/13 19:10:23 mgrouch Exp $ */

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
    fprintf(o, "      ed   (or edit)      - Edit XML document\n");
    fprintf(o, "      sel  (or select)    - Select data or query XML document(s)\n");
    fprintf(o, "      tr   (or transform) - Transform XML document\n");
    fprintf(o, "      val  (or validate)  - Validate XML document(s)\n");
#if 0
    /* TODO */
    fprintf(o, "      diff                - Find differences between XML documents\n");
#endif

    fprintf(o, "Type: %s <command> --help <ENTER> for more options\n\n", cmd);
    fprintf(o, "XMLStarlet is a command line toolkit to query/edit/check/transform\n");
    fprintf(o, "XML documents (for more information see http://xmlstar.sourceforge.net/)\n");

    if (dirc)
    {
        free(dirc);
        cmd = 0;
    }

    exit(1);
}  

int main(int argc, char **argv)
{
    int ret = 0;

    if (argc <= 1) usage(argc, argv);
    if (!strcmp(argv[1], "ed") || !strcmp(argv[1], "edit"))
    {
        ret = xml_edit(argc, argv);
    } else
    if (!strcmp(argv[1], "sel") || !strcmp(argv[1], "select"))
    {
        ret = xml_select(argc, argv);
    } else
    if (!strcmp(argv[1], "tr") || !strcmp(argv[1], "transform"))
    {
        ret = xml_trans(argc, argv);
    } else
    if (!strcmp(argv[1], "val") || !strcmp(argv[1], "validate"))
    {
        ret = xml_validate(argc, argv);
    } else
    {
        usage(argc, argv);
    }
    
    exit(ret);
}


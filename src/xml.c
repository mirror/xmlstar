/* $Id: xml.c,v 1.11 2002/11/23 05:33:28 mgrouch Exp $ */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char usage_str[] =
"XMLStarlet Toolkit: Command line utilities for XML\n"
"Usage: xml <command> [<options>]\n"
"where <command> is one of:\n"
"   ed   (or edit)      - Edit XML document(s)\n"
"   sel  (or select)    - Select data or query XML document(s)\n"
"   tr   (or transform) - Transform XML document(s)\n"
"   val  (or validate)  - Validate XML document(s)\n"
#if 0
/* TODO */
"   diff                - Find differences between XML documents\n"
#endif

"Type: xml <command> --help <ENTER> for command help\n\n"
"XMLStarlet is a command line toolkit to query/edit/check/transform\n"
"XML documents (for more information see http://xmlstar.sourceforge.net/)\n";

void usage(int argc, char **argv)
{
    FILE* o = stderr;
    fprintf(o, usage_str);
    exit(1);
}  

int main(int argc, char **argv)
{
    int ret = 0;

    extern int xml_edit(int argc, char **argv);
    extern int xml_select(int argc, char **argv);
    extern int xml_trans(int argc, char **argv);
    extern int xml_validate(int argc, char **argv);
    
    if (argc <= 1) usage(argc, argv);

    if (!strcmp(argv[1], "ed") || !strcmp(argv[1], "edit"))
    {
        ret = xml_edit(argc, argv);
    }
    else if (!strcmp(argv[1], "sel") || !strcmp(argv[1], "select"))
    {
        ret = xml_select(argc, argv);
    }
    else if (!strcmp(argv[1], "tr") || !strcmp(argv[1], "transform"))
    {
        ret = xml_trans(argc, argv);
    }
    else if (!strcmp(argv[1], "val") || !strcmp(argv[1], "validate"))
    {
        ret = xml_validate(argc, argv);
    }
    else
    {
        usage(argc, argv);
    }
    
    exit(ret);
}

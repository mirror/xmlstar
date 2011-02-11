/*  $Id: xml.c,v 1.37 2004/11/11 03:39:34 mgrouch Exp $  */

/*

XMLStarlet: Command Line Toolkit to query/edit/check/transform XML documents

Copyright (c) 2002 Mikhail Grushinskiy.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <config.h>
#include <version.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "xmlstar.h"

extern int edMain(int argc, char **argv);
extern int selMain(int argc, char **argv);
extern int trMain(int argc, char **argv);
extern int valMain(int argc, char **argv);
extern int foMain(int argc, char **argv);
extern int elMain(int argc, char **argv);
extern int c14nMain(int argc, char **argv);
extern int lsMain(int argc, char **argv);
extern int pyxMain(int argc, char **argv);
extern int depyxMain(int argc, char **argv);
extern int escMain(int argc, char **argv, int escape);

/* 
 * usage string chunk : 509 char max on ISO C90
 */
static const char usage_str_1[] = 
"XMLStarlet Toolkit: Command line utilities for XML\n"
"Usage: %s [<options>] <command> [<cmd-options>]\n";

static const char usage_str_2[] = 
"where <command> is one of:\n"
"  ed    (or edit)      - Edit/Update XML document(s)\n"
"  sel   (or select)    - Select data or query XML document(s) (XPATH, etc)\n"
"  tr    (or transform) - Transform XML document(s) using XSLT\n"
"  val   (or validate)  - Validate XML document(s) (well-formed/DTD/XSD/RelaxNG)\n"
"  fo    (or format)    - Format XML document(s)\n"
"  el    (or elements)  - Display element structure of XML document\n";

static const char usage_str_3[] = 
"  c14n  (or canonic)   - XML canonicalization\n"
"  ls    (or list)      - List directory as XML\n"
"  esc   (or escape)    - Escape special XML characters\n"
"  unesc (or unescape)  - Unescape special XML characters\n"
"  pyx   (or xmln)      - Convert XML into PYX format (based on ESIS - ISO 8879)\n"
"  p2x   (or depyx)     - Convert PYX into XML\n";

static const char usage_str_4[] = 
"<options> are:\n"
"  --version            - show version\n"
"  --help               - show help\n"
"Wherever file name mentioned in command help it is assumed\n"
"that URL can be used instead as well.\n\n"
"Type: %s <command> --help <ENTER> for command help\n\n";



const char more_info[] =
"XMLStarlet is a command line toolkit to query/edit/check/transform\n"
"XML documents (for more information see http://xmlstar.sourceforge.net/)\n";

const char libxslt_more_info[] =
"\n"
"Current implementation uses libxslt from GNOME codebase as XSLT processor\n"
"(see http://xmlsoft.org/ for more details)\n";

/**
 *  Display usage syntax
 */
void
usage(int argc, char **argv, exit_status status)
{
    FILE* o = (status == EXIT_SUCCESS)? stdout : stderr;

    fprintf(o, usage_str_1, argv[0]);
    fprintf(o, "%s", usage_str_2);
    fprintf(o, "%s", usage_str_3);
    fprintf(o, usage_str_4, argv[0]);
    fprintf(o, "%s", more_info);
    exit(status);
}  

/**
 *  This is the main function
 */
int
main(int argc, char **argv)
{
    int ret = 0;

    if (argc <= 1)
    {
        usage(argc, argv, EXIT_BAD_ARGS);
    }
    else if (!strcmp(argv[1], "ed") || !strcmp(argv[1], "edit"))
    {
        ret = edMain(argc, argv);
    }
    else if (!strcmp(argv[1], "sel") || !strcmp(argv[1], "select"))
    {
        ret = selMain(argc, argv);
    }
    else if (!strcmp(argv[1], "tr") || !strcmp(argv[1], "transform"))
    {
        ret = trMain(argc, argv);
    }
    else if (!strcmp(argv[1], "fo") || !strcmp(argv[1], "format"))
    {
        ret = foMain(argc, argv);
    }
    else if (!strcmp(argv[1], "val") || !strcmp(argv[1], "validate"))
    {
        ret = valMain(argc, argv);
    }
    else if (!strcmp(argv[1], "el") || !strcmp(argv[1], "elements"))
    {
        ret = elMain(argc, argv);
    }
    else if (!strcmp(argv[1], "c14n") || !strcmp(argv[1], "canonic"))
    {
        ret = c14nMain(argc, argv);
    }
    else if (!strcmp(argv[1], "ls") || !strcmp(argv[1], "list"))
    {
        ret = lsMain(argc, argv);
    }
    else if (!strcmp(argv[1], "pyx") || !strcmp(argv[1], "xmln"))
    {
        ret = pyxMain(argc, argv);
    }
    else if (!strcmp(argv[1], "depyx") || !strcmp(argv[1], "p2x"))
    {
        ret = depyxMain(argc, argv);
    }
    else if (!strcmp(argv[1], "esc") || !strcmp(argv[1], "escape"))
    {
        ret = escMain(argc, argv, 1);
    }
    else if (!strcmp(argv[1], "unesc") || !strcmp(argv[1], "unescape"))
    {
        ret = escMain(argc, argv, 0);
    }
    else if (!strcmp(argv[1], "--version"))
    {
        fprintf(stdout, "%s\n", VERSION);
        ret = EXIT_SUCCESS;
    }
    else
    {
        usage(argc, argv, strcmp(argv[1], "--help") == 0?
            EXIT_SUCCESS : EXIT_BAD_ARGS);
    }
    
    exit(ret);
}

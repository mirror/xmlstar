/*  $Id: xml_trans.c,v 1.38 2005/01/07 02:40:59 mgrouch Exp $  */

/*

XMLStarlet: Command Line Toolkit to query/edit/check/transform XML documents

Copyright (c) 2002-2004 Mikhail Grushinskiy.  All Rights Reserved.

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "xmlstar.h"
#include "trans.h"

/*
 *  TODO:
 *        1. proper command line arguments handling
 *        2. review and clean up all code (free memory)
 *        3. check embedded stylesheet support
 *        4. exit values on errors
 */

/**
 *  Display usage syntax
 */
void
trUsage(exit_status status)
{
    extern void fprint_trans_usage(FILE* o, const char* argv0);
    extern const char more_info[];
    extern const char libxslt_more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprint_trans_usage(o, get_arg(ARG0));
    fprintf(o, "%s", more_info);
    fprintf(o, "%s", libxslt_more_info);
    exit(status);
}

/**
 *  Parse global command line options
 */
void
trParseOptions(xsltOptionsPtr ops)
{
    for (;;) {
        const char* arg = get_arg(OPTION_NEXT);
        if (!arg) break;

        if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            trUsage(EXIT_SUCCESS);
        } else if (strcmp(arg, "--show-ext") == 0) {
            ops->show_extensions = 1;
        } else if (strcmp(arg, "--val") == 0) {
            ops->noval = 0;
        } else if (strcmp(arg, "--net") == 0) {
            ops->nonet = 0;
        } else if (strcmp(arg, "-E") == 0 || strcmp(arg, "--embed") == 0) {
            ops->embed = 1;
        } else if (strcmp(arg, "--omit-decl") == 0) {
            ops->omit_decl = 1;
        } else if (strcmp(arg, "--maxdepth") == 0) {
            int value;
            arg = get_arg(ARG_NEXT);
            if (!arg) {
                fprintf(stderr, "--maxdepth requires <val>\n");
                exit(EXIT_BAD_ARGS);
            }
            if (sscanf(arg, "%d", &value) == 1)
                if (value > 0) xsltMaxDepth = value;
#ifdef LIBXML_XINCLUDE_ENABLED
        } else if (strcmp(arg, "--xinclude") == 0) {
            ops->xinclude = 1;
#endif
#ifdef LIBXML_HTML_ENABLED
        } else if (strcmp(arg, "--html") == 0) {
            ops->html = 1;
#endif
        } else {
            fprintf(stderr, "Warning: unrecognized option %s\n", arg);
        }
    }
}

/**
 *  Cleanup memory
 */
void
trCleanup()
{
    xsltCleanupGlobals();
    xmlCleanupParser();
#if 0
    xmlMemoryDump();
#endif
}

/**
 *  Parse command line for XSLT parameters
 */
void
trParseParams(const char** params, int* plen)
{
    *plen = 0;
    params[0] = 0;

    for (;;) {
        const char* arg = get_arg(OPTION_NEXT);
        if (!arg) break;

        if (strcmp(arg, "-p") == 0) {
            int j;
            xmlChar *name, *value;

            arg = get_arg(ARG_NEXT);
            if (!arg) {
                fprintf(stderr, "-p must be followed by <name>=<value>\n");
                exit(EXIT_BAD_ARGS);
            }

            for(j=0; arg[j] && (arg[j] != '='); j++);
            if (arg[j] != '=') trUsage(EXIT_BAD_ARGS);

            name = xmlStrndup((const xmlChar *) arg, j);
            value = xmlStrdup((const xmlChar *) arg+j+1);

            if (*plen >= MAX_PARAMETERS)
            {
                fprintf(stderr, "too many params increase MAX_PARAMETERS\n");
                exit(EXIT_INTERNAL_ERROR);
            }

            params[*plen] = (char *)name;
            (*plen)++;
            params[*plen] = (char *)value;
            (*plen)++;
            params[*plen] = 0;
        } else if (strcmp(arg, "-s") == 0) {
            int j;
            const xmlChar *string;
            xmlChar *name, *value;

            arg = get_arg(ARG_NEXT);
            if (!arg) {
                fprintf(stderr, "-s must be followed by <name>=<value>\n");
                exit(EXIT_BAD_ARGS);
            }

            for(j=0; arg[j] && (arg[j] != '='); j++);
            if (arg[j] != '=') trUsage(EXIT_BAD_ARGS);

            name = xmlStrndup((const xmlChar *)arg, j);
            string = (const xmlChar *)(arg+j+1);

            if (xmlStrchr(string, '"'))
            {
                if (xmlStrchr(string, '\''))
                {
                    fprintf(stderr,
                        "string parameter contains both quote and double-quotes\n");
                    exit(EXIT_INTERNAL_ERROR);
                }
                value = xmlStrdup((const xmlChar *)"'");
                value = xmlStrcat(value, string);
                value = xmlStrcat(value, (const xmlChar *)"'");
            }
            else
            {
                value = xmlStrdup((const xmlChar *)"\"");
                value = xmlStrcat(value, string);
                value = xmlStrcat(value, (const xmlChar *)"\"");
            }

            if (*plen >= MAX_PARAMETERS)
            {
                fprintf(stderr, "too many params increase MAX_PARAMETERS\n");
                exit(EXIT_INTERNAL_ERROR);
            }

            params[*plen] = (char *)name;
            (*plen)++;
            params[*plen] = (char *)value;
            (*plen)++;
            params[*plen] = 0;
        }
    }
}

/**
 *  Cleanup memory allocated by XSLT parameters
 */
void
trCleanupParams(const char **xsltParams)
{
    const char **p = xsltParams;

    while (*p)
    {
        xmlFree((char *)*p);
        p++;
    }
}

/**
 *  This is the main function for 'tr' option
 */
int
trMain(void)
{
    static xsltOptions ops;
    static const char *xsltParams[2 * MAX_PARAMETERS + 1];

    int errorno = 0;
    const char* xslt_filename;
    int pCount;
    
    xsltInitOptions(&ops);
    trParseOptions(&ops);
    xslt_filename = get_arg(ARG_NEXT);
    if (!xslt_filename) trUsage(EXIT_BAD_ARGS);
    xsltInitLibXml(&ops);

    /* set parameters */
    trParseParams(xsltParams, &pCount);
    
    /* run transformation */
    errorno = xsltRun(&ops, xslt_filename, xsltParams,
        main_argc - main_argi, main_argv + main_argi);

    /* free resources */
    trCleanupParams(xsltParams);
    trCleanup();
    
    return errorno;                                                
}

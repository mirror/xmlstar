/*  $Id: xml_select.c,v 1.65 2004/11/30 01:59:34 mgrouch Exp $  */

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

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "trans.h"
#include "stack.h"

/*
 *  TODO:
 *
 *   1. free memory (check for memory leaks)
 *   2. --nonet option (to disable fetching DTD over network)
 */

#define MAX_XSL_BUF    256*1024
#define MAX_NS_ARGS    256

char xsl_buf[MAX_XSL_BUF];

typedef struct _selOptions {
    int printXSLT;            /* Display prepared XSLT */
    int printRoot;            /* Print root element in output (if XML) */
    int outText;              /* Output is text */
    int indent;               /* Indent output */
    int noblanks;             /* Remove insignificant spaces from XML tree */
    int no_omit_decl;         /* Print XML declaration line <?xml version="1.0"?> */
    int nonet;                /* refuse to fetch DTDs or entities over network */
    const char *encoding;     /* the "encoding" attribute on the stylesheet's <xsl:output/> */
} selOptions;

typedef selOptions *selOptionsPtr;

/*
 * usage string chunk : 509 char max on ISO C90
 */
static const char select_usage_str_1[] =
"XMLStarlet Toolkit: Select from XML document(s)\n"
"Usage: xml sel <global-options> {<template>} [ <xml-file> ... ]\n"
"where\n"
"  <global-options> - global options for selecting\n"
"  <xml-file> - input XML document file name/uri (stdin is used if missing)\n"
"  <template> - template for querying XML document with following syntax:\n\n";

static const char select_usage_str_2[] =
"<global-options> are:\n"
"  -C or --comp              - display generated XSLT\n"
"  -R or --root              - print root element <xsl-select>\n"
"  -T or --text              - output is text (default is XML)\n"
"  -I or --indent            - indent output\n"
"  -D or --xml-decl          - do not omit xml declaration line\n"
"  -B or --noblanks          - remove insignificant spaces from XML tree\n"
"  -E or --encode <encoding> - output in the given encoding (utf-8, unicode...)\n";

static const char select_usage_str_3[] =
"  -N <name>=<value>         - predefine namespaces (name without \'xmlns:\')\n"
"                              ex: xsql=urn:oracle-xsql\n"
"                              Multiple -N options are allowed.\n"
"  --net                     - allow fetch DTDs or entities over network\n"
"  --help                    - display help\n\n";

static const char select_usage_str_4[] =
"Syntax for templates: -t|--template <options>\n"
"where <options>\n"
"  -c or --copy-of <xpath>   - print copy of XPATH expression\n"
"  -v or --value-of <xpath>  - print value of XPATH expression\n"
"  -o or --output <string>   - output string literal\n"
"  -n or --nl                - print new line\n"
"  -f or --inp-name          - print input file name (or URL)\n";

static const char select_usage_str_5[] =
"  -m or --match <xpath>     - match XPATH expression\n"
"  -i or --if <test-xpath>   - check condition <xsl:if test=\"test-xpath\">\n"
"  -e or --elem <name>       - print out element <xsl:element name=\"name\">\n"
"  -a or --attr <name>       - add attribute <xsl:attribute name=\"name\">\n"
"  -b or --break             - break nesting\n"
"  -s or --sort op xpath     - sort in order (used after -m) where\n";

static const char select_usage_str_6[] =
"  op is X:Y:Z, \n"
"      X is A - for order=\"ascending\"\n"
"      X is D - for order=\"descending\"\n"
"      Y is N - for data-type=\"numeric\"\n"
"      Y is T - for data-type=\"text\"\n"
"      Z is U - for case-order=\"upper-first\"\n"
"      Z is L - for case-order=\"lower-first\"\n\n";

static const char select_usage_str_7[] =
"There can be multiple --match, --copy-of, --value-of, etc options\n"
"in a single template. The effect of applying command line templates\n"
"can be illustrated with the following XSLT analogue\n\n"

"xml sel -t -c \"xpath0\" -m \"xpath1\" -m \"xpath2\" -v \"xpath3\" \\\n"
"        -t -m \"xpath4\" -c \"xpath5\"\n\n"

"is equivalent to applying the following XSLT\n\n";

static const char select_usage_str_8[] =
"<?xml version=\"1.0\"?>\n"
"<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n"
"<xsl:template match=\"/\">\n"
"  <xsl:call-template name=\"t1\"/>\n"
"  <xsl:call-template name=\"t2\"/>\n"
"</xsl:template>\n"
"<xsl:template name=\"t1\">\n"
"  <xsl:copy-of select=\"xpath0\"/>\n"
"  <xsl:for-each select=\"xpath1\">\n"
"    <xsl:for-each select=\"xpath2\">\n"
"      <xsl:value-of select=\"xpath3\"/>\n"
"    </xsl:for-each>\n"
"  </xsl:for-each>\n"
"</xsl:template>\n";

static const char select_usage_str_9[] =
"<xsl:template name=\"t2\">\n"
"  <xsl:for-each select=\"xpath4\">\n"
"    <xsl:copy-of select=\"xpath5\"/>\n"
"  </xsl:for-each>\n"
"</xsl:template>\n"
"</xsl:stylesheet>\n\n";

/**
 *  Print small help for command line options
 */
void
selUsage(int argc, char **argv)
{
    extern const char more_info[];
    extern const char libxslt_more_info[];
    FILE* o = stderr;
    fprintf(o, "%s", select_usage_str_1);
    fprintf(o, "%s", select_usage_str_2);
    fprintf(o, "%s", select_usage_str_3);
    fprintf(o, "%s", select_usage_str_4);
    fprintf(o, "%s", select_usage_str_5);
    fprintf(o, "%s", select_usage_str_6);
    fprintf(o, "%s", select_usage_str_7);
    fprintf(o, "%s", select_usage_str_8);
    fprintf(o, "%s", select_usage_str_9);
    fprintf(o, "%s", more_info);
    fprintf(o, "%s", libxslt_more_info);
    exit(1);
}

/**
 *  Initialize global command line options
 */
void
selInitOptions(selOptionsPtr ops)
{
    ops->printXSLT = 0;
    ops->printRoot = 0;
    ops->outText = 0;
    ops->indent = 0;
    ops->noblanks = 0;
    ops->no_omit_decl = 0;
    ops->nonet = 1;
    ops->encoding = NULL;
}

/**
 *  Parse command line for additional namespaces
 */
int
selParseNSArr(const char** ns_arr, int* plen,
              int count, char **argv)
{
    int i = 0;
    *plen = 0;
    ns_arr[0] = 0;

    for (i=0; i<count; i++)
    {
        if (argv[i] == 0) break;
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "-N"))
            {
                int j;
                xmlChar *name, *value;

                i++;
                if (i >= count) selUsage(0, NULL);

                for(j=0; argv[i][j] && (argv[i][j] != '='); j++);
                if (argv[i][j] != '=') selUsage(0, NULL);

                name = xmlStrndup((const xmlChar *) argv[i], j);
                value = xmlStrdup((const xmlChar *) argv[i]+j+1);

                if (*plen >= MAX_NS_ARGS)
                {
                    fprintf(stderr, "too many namespaces increase MAX_NS_ARGS\n");
                    exit(2);
                }

                ns_arr[*plen] = (char *)name;
                (*plen)++;
                ns_arr[*plen] = (char *)value;
                (*plen)++;
                ns_arr[*plen] = 0;

                /*printf("xmlns:%s=\"%s\"\n", name, value);*/
            }
        }
        else
            break;
    }

    return i;
}

/**
 *  Cleanup memory allocated by namespaces arguments
 */
void
selCleanupNSArr(const char **ns_arr)
{
    const char **p = ns_arr;

    while (*p)
    {
        xmlFree((char *)*p);
        p++;
    }
}

/**
 *  Parse global command line options
 */
int
selParseOptions(selOptionsPtr ops, int argc, char **argv)
{
    int i;

    i = 2;
    while((i < argc) && (strcmp(argv[i], "-t")) && strcmp(argv[i], "--template"))
    {
        if (!strcmp(argv[i], "-C"))
        {
            ops->printXSLT = 1;
        }
        else if (!strcmp(argv[i], "-B") || !strcmp(argv[i], "--noblanks"))
        {
            ops->noblanks = 1;
        }
        else if (!strcmp(argv[i], "-T") || !strcmp(argv[i], "--text"))
        {
            ops->outText = 1;
        }
        else if (!strcmp(argv[i], "-R") || !strcmp(argv[i], "--root"))
        {
            ops->printRoot = 1;
        }
        else if (!strcmp(argv[i], "-I") || !strcmp(argv[i], "--indent"))
        {
            ops->indent = 1;
        }
        else if (!strcmp(argv[i], "-D") || !strcmp(argv[i], "--xml-decl"))
        {
            ops->no_omit_decl = 1;
        }
        else if (!strcmp(argv[i], "-E") || !strcmp(argv[i], "--encode"))
        {
            if ((i+1) < argc)
            {
                if (argv[i + 1][0] == '-')
                {
                    fprintf(stderr, "-E option requires argument <encoding> ex: (utf-8, unicode...)\n");
                    exit(2);
                }
                else
                {
                    ops->encoding = argv[i + 1];
                }
            }
            else
            {
                fprintf(stderr, "-E option requires argument <encoding> ex: (utf-8, unicode...)\n");
                exit(2);
            }

        }
        else if (!strcmp(argv[i], "--net"))
        {
            ops->nonet = 0;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") ||
                 !strcmp(argv[i], "-?") || !strcmp(argv[i], "-Z"))
        {
            selUsage(argc, argv);
        }
        i++;
    }

    return i;
}

#define STK_MATCH 'm'
#define STK_IF    'i'
#define STK_ELEM  'e'
#define STK_ATTR  'a'

/**
 *  Prepare XSLT template based on command line options
 *  Assumes start points to -t option
 */
int
selGenTemplate(char* xsl_buf, int *len, selOptionsPtr ops, int num,
               int* lastTempl, int start, int argc, char **argv)
{
    int c, i, j, /*k,*/ m, t;
    int templateEmpty = 1;
    int nextTempl = 0;

    Stack stack = NULL;
    int max_depth = 256;  /* TODO: make it optional */

    c = *len;
    i = start;
    t = num;

    *lastTempl = 0;

    if (strcmp(argv[i], "-t") && strcmp(argv[i], "--template"))
    {
        fprintf(stderr, "not at the beginning of template\n");
        abort();
    }

    templateEmpty = 1;
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:template name=\"t%d\">\n", t);

    stack = stack_create(max_depth);

    i++;
    m = 0;
    while(i < argc)
    {
        if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--copy-of"))
        {
            templateEmpty = 0;
            for (j=0; j <= m; j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
            c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:copy-of select=\"%s\"/>\n", argv[i+1]);
            i++;
        }
        else if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--value-of"))
        {
            templateEmpty = 0;
            if ((i+1) >= argc)
            {
                fprintf(stderr, "-v option requires argument\n");
                stack_free(stack);
                exit (1);
            }
            for (j=0; j <= m; j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
            c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:value-of select=\"%s\"/>\n", argv[i+1]);
            i++;
        }
        else if(!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
        {
            templateEmpty = 0;
            if ((i+1) >= argc)
            {
                fprintf(stderr, "-o option requires argument\n");
                stack_free(stack);
                exit (1);
            }
            for (j=0; j <= m; j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
            c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:value-of select=\"'%s'\"/>\n", argv[i+1]);
            i++;
        }
        else if(!strcmp(argv[i], "-f") || !strcmp(argv[i], "--inp-name"))
        {
            templateEmpty = 0;
            for (j=0; j <= m; j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
            c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:copy-of select=\"$inputFile\"/>\n");
        }
        else if(!strcmp(argv[i], "-n") || !strcmp(argv[i], "--nl"))
        {
            templateEmpty = 0;
            for (j=0; j <= m; j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
            c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:value-of select=\"'&#10;'\"/>\n");
        }
        else if(!strcmp(argv[i], "-i") || !strcmp(argv[i], "--if"))
        {
            templateEmpty = 0;
            if ((i+1) >= argc)
            {
                fprintf(stderr, "-i option requires argument\n");
                stack_free(stack);
                exit (1);
            }
            for (j=0; j <= m; j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
            c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:if test=\"%s\">\n", argv[i+1]);
            stack_push(stack, STK_IF);
            m++;
            i++;
        }
        else if(!strcmp(argv[i], "-e") || !strcmp(argv[i], "--elem"))
        {
            templateEmpty = 0;
            if ((i+1) >= argc)
            {
                fprintf(stderr, "-e option requires argument\n");
                stack_free(stack);
                exit (1);
            }
            for (j=0; j <= m; j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
            c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:element name=\"%s\">\n", argv[i+1]);
            stack_push(stack, STK_ELEM);
            m++;
            i++;
        }
        else if(!strcmp(argv[i], "-a") || !strcmp(argv[i], "--attr"))
        {
            templateEmpty = 0;
            if ((i+1) >= argc)
            {
                fprintf(stderr, "-a option requires argument\n");
                stack_free(stack);
                exit (1);
            }
            for (j=0; j <= m; j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
            c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:attribute name=\"%s\">\n", argv[i+1]);
            stack_push(stack, STK_ATTR);
            m++;
            i++;
        }
        else if(!strcmp(argv[i], "-m") || !strcmp(argv[i], "--match"))
        {
            templateEmpty = 0;
            if ((i+1) >= argc)
            {
                fprintf(stderr, "-m option requires argument\n");
                stack_free(stack);
                exit (1);
            }
            for (j=0; j <= m; j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
            c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:for-each select=\"%s\">\n", argv[i+1]);
            stack_push(stack, STK_MATCH);
            m++;
            i++;
            if ((i+1 < argc) && (!strcmp(argv[i+1], "-s") || !strcmp(argv[i+1], "--sort")))
            {
                char Order, Type, Case, *Select;
                i++;
                if ((i+1) >= argc)
                {
                    fprintf(stderr, "-s missing argument\n");
                    stack_free(stack);
                    exit (1);
                }
                i++;

                /* TODO: more validation here */
                Order = argv[i][0];
                Type = argv[i][2];
                Case = argv[i][4];

                if ((i+1) >= argc)
                {
                    fprintf(stderr, "-s missing argument\n");
                    stack_free(stack);
                    exit (1);
                }
                i++;
                Select=argv[i];

                for (j=0; j <= m; j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");

                if (Order == 'A') c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:sort order=\"ascending\"");
                else c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:sort order=\"descending\"");

                if (Type == 'N') c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, " data-type=\"number\"");
                else c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, " data-type=\"text\"");

                if (Case == 'L') c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, " case-order=\"lower-first\"");
                else c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, " case-order=\"upper-first\"");

                c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, " select=\"%s\"", Select);

                c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "/>\n");
            }
        }
        else if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template"))
        {
            nextTempl = 1;
            i--;
            break;
        }
        else if(!strcmp(argv[i], "-b") || !strcmp(argv[i], "--break"))
        {
            if(!stack_isEmpty(stack))
            {
                StackItem itm;

                for (j=0; j<stack_depth(stack); j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
                itm = stack_pop(stack);
                if (itm == STK_MATCH)
                {
                    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xsl:for-each>\n");
                    m--;
                }
                else if (itm == STK_IF)
                {
                    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xsl:if>\n");
                    m--;
                }
                else if (itm == STK_ELEM)
                {
                    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xsl:element>\n");
                    m--;
                }
                else if (itm == STK_ATTR)
                {
                    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xsl:attribute>\n");
                    m--;
                }
                /* printf("%c\n", itm); */
            }
        }
        else
        {
            if (argv[i][0] != '-')
            {
                break;
            }
            else if (!strcmp(argv[i], "-"))
            {
                break;
            }
            else
            {
                fprintf(stderr, "unknown option: %s\n", argv[i]);
                stack_free(stack);
                exit(1);
            }
        }

        i++;
    }
/*
    if((i < argc) && (!strcmp(argv[i], "-n") || !strcmp(argv[i], "--nl")))
    {
        for (j=0; j <= m; j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
        c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:value-of select=\"'&#10;'\"/>\n");
    }
*/

    while(!stack_isEmpty(stack))
    {
        StackItem itm;

        for (j=0; j<stack_depth(stack); j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
        itm = stack_pop(stack);
        if (itm == STK_MATCH) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xsl:for-each>\n");
        else if (itm == STK_IF) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xsl:if>\n");
        else if (itm == STK_ELEM) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xsl:element>\n");
        else if (itm == STK_ATTR) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xsl:attribute>\n");
/*        printf("%c\n", itm);  */
    }
/*
    for (k=0; k<m; k++)
    {
        for (j=k; j<m; j++) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  ");
        c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xsl:for-each>\n");
    }
*/
    if (templateEmpty)
    {
        fprintf(stderr, "error in arguments:");
        fprintf(stderr, " -t or --template option must be followed by");
        fprintf(stderr, " --match or other options\n");
        stack_free(stack);
        exit(3);
    }

    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xsl:template>\n");

    *len = c;

    stack_free(stack);

    if (!nextTempl)
    {
        if (i >= argc)
        {
            *lastTempl = 1;
            return i;
        }
        if (argv[i][0] != '-')
        {
            *lastTempl = 1;
            return i;
        }
        if (!strcmp(argv[i], "-"))
        {
            *lastTempl = 1;
            return i;
        }
    }

    /* return index of the beginning of the next template or input file name */
    i++;
    return i;
}

/**
 *  Prepare XSLT stylesheet based on command line options
 */
int
selPrepareXslt(char* xsl_buf, int *len, selOptionsPtr ops, const char *ns_arr[],
               int start, int argc, char **argv)
{
    int c, i, t, ns;

    xsl_buf[0] = 0;
    *len = 0;
    c = 0;

    c += sprintf(xsl_buf, "<?xml version=\"1.0\"?>\n");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1,
      "<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:exslt=\"http://exslt.org/common\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:math=\"http://exslt.org/math\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:date=\"http://exslt.org/dates-and-times\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:func=\"http://exslt.org/functions\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:set=\"http://exslt.org/sets\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:str=\"http://exslt.org/strings\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:dyn=\"http://exslt.org/dynamic\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:saxon=\"http://icl.com/saxon\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:xalanredirect=\"org.apache.xalan.xslt.extensions.Redirect\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:xt=\"http://www.jclark.com/xt\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:libxslt=\"http://xmlsoft.org/XSLT/namespace\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:test=\"http://xmlsoft.org/XSLT/\"");

    ns = 0;
    while(ns_arr[ns])
    {
        if (strlen(ns_arr[ns]))
           c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns:%s=\"%s\"", ns_arr[ns], ns_arr[ns+1]);
        else
           c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n xmlns=\"%s\"", ns_arr[ns+1]);
        ns += 2;
    }
    selCleanupNSArr(ns_arr);

    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1,
                  "\n extension-element-prefixes=\"exslt math date func set str dyn saxon xalanredirect xt libxslt test\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "\n exclude-result-prefixes=\"math str\"");


    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, ">\n");

    if (ops->no_omit_decl) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:output omit-xml-declaration=\"no\"");
    else c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:output omit-xml-declaration=\"yes\"");
    if (ops->indent) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, " indent=\"yes\"");
    else c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, " indent=\"no\"");
    if (ops->encoding != NULL) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, " encoding=\"%s\"", ops->encoding);
    if (ops->outText) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, " method=\"text\"");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "/>\n");

    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:param name=\"inputFile\">-</xsl:param>\n");

    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xsl:template match=\"/\">\n");
    if (!ops->outText && ops->printRoot) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "<xml-select>\n");

    t = 0;
    i = start;
    while(i < argc)
    {
        if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template"))
        {
            t++;
            c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "  <xsl:call-template name=\"t%d\"/>\n", t);
        }
        i++;
    }
    if (!ops->outText && ops->printRoot) c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xml-select>\n");
    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xsl:template>\n");

    /*
     *  At least one -t option must be found
     */
    if (t == 0)
    {
        fprintf(stderr, "error in arguments:");
        fprintf(stderr, " no -t or --template options found\n");
        exit(2);
    }

    t = 0;
    i = start;
    while(i < argc)
    {
        if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template"))
        {
            int lastTempl = 0;
            t++;
            i = selGenTemplate(xsl_buf, &c, ops, t, &lastTempl, i, argc, argv);
            if (lastTempl) break;
        }
    }

    c += snprintf(xsl_buf + c, MAX_XSL_BUF - c - 1, "</xsl:stylesheet>\n");
    *len = c;

    return i;
}

/**
 *  This is the main function for 'select' option
 */
int
selMain(int argc, char **argv)
{
    static xsltOptions xsltOps;
    static selOptions ops;
    static const char *params[2 * MAX_PARAMETERS + 1];
    static const char *ns_arr[2 * MAX_NS_ARGS + 1];
    int start, c, i, n;
    int nCount = 0;
    int nbparams;

    if (argc <= 2) selUsage(argc, argv);

    selInitOptions(&ops);
    xsltInitOptions(&xsltOps);
    start = selParseOptions(&ops, argc, argv);
    xsltOps.nonet = ops.nonet;
    xsltOps.noblanks = ops.noblanks;
    xsltInitLibXml(&xsltOps);

    /* set parameters */
    selParseNSArr(ns_arr, &nCount, argc-start, argv+2);

    c = sizeof(xsl_buf);
    i = selPrepareXslt(xsl_buf, &c, &ops, ns_arr, start, argc, argv);

    if (ops.printXSLT)
    {
        fprintf(stdout, "%s", xsl_buf);
        exit(0);
    }

    for (n=i; n<argc; n++)
    {
        xmlChar *value;

        /*
         *  Pass input file name as predefined parameter 'inputFile'
         */
        nbparams = 2;
        params[0] = "inputFile";
        value = xmlStrdup((const xmlChar *)"'");
        value = xmlStrcat((xmlChar *)value, (const xmlChar *)argv[n]);
        value = xmlStrcat((xmlChar *)value, (const xmlChar *)"'");
        params[1] = (char *) value;

        /*
         *  Parse XSLT stylesheet
         */
        {
            xmlDocPtr style = xmlParseMemory(xsl_buf, c);
            xsltStylesheetPtr cur = xsltParseStylesheetDoc(style);
            xmlDocPtr doc = NULL;
            if (!cur) exit(2);
            doc = xmlParseFile(argv[n]);
            if (doc != NULL) {
                xsltProcess(&xsltOps, doc, params, cur, argv[n]);
            }
            xsltFreeStylesheet(cur);
        }
    }

    if (i == argc)
    {
        /*
         *  Parse XSLT stylesheet
         */
        xmlDocPtr style = xmlParseMemory(xsl_buf, c);
        xsltStylesheetPtr cur = xsltParseStylesheetDoc(style);
        xmlDocPtr doc = NULL;
        if (!cur) exit(2);
        doc = xmlParseFile("-");
        if (doc != NULL) {
            xsltProcess(&xsltOps, doc, params, cur, "-");
        }
        /* xsltFreeStylesheet(cur); */
    }

    return 0;
}


/*  $Id: xml_select.c,v 1.35 2002/12/07 21:49:39 mgrouch Exp $  */

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "trans.h"

/*
 *  TODO:
 *
 *   1. How about sorting ?
 *   2. strip spaces
 *   3. How to list files which match templates ?
 *   4. free memory
 */

#define MAX_XSL_BUF  256*1024

char xsl_buf[MAX_XSL_BUF];

typedef struct _selOptions {
    int printXSLT;            /* Display prepared XSLT */
    int printRoot;            /* Print root element in output (if XML) */
    int outText;              /* Output is text */
    int indent;               /* Indent output */
    int no_omit_decl;         /* Print XML declaration line <?xml version="1.0"?> */
} selOptions;

typedef selOptions *selOptionsPtr;

static const char select_usage_str[] =
"XMLStarlet Toolkit: Select from XML document(s)\n"
"Usage: xml sel <global-options> {<template>} [ <xml-file> ... ]\n"
"where\n"
"  <global-options> - global options for selecting\n"
"  <xml-file> - input XML document file name (stdin is used if missing)\n"
"  <template> - template for querying XL document with following syntax:\n\n"

"<global-options> are:\n"
"  -C                 - display generated XSLT\n"
"  -R                 - print root element <xsl-select>\n"
"  -T                 - output is text (default is XML)\n"
"  -I                 - indent output\n"
"  -D                 - do not omit xml declaration line\n"
"  --help             - display help\n\n"

"Syntax for templates: -t|--template <options>\n"
"where <options>\n"
"  -c or --copy-of <xpath>  - print copy of XPATH expression\n"
"  -v or --value-of <xpath> - print value of XPATH expression\n"
"  -o or --output <string>  - print string literal \n"
"  -n or --nl               - print new line\n"
"  -s or --sort <order>     - sort in order (used after -m)\n"
"  -m or --match <xpath>    - match XPATH expression\n\n"
"There can be multiple --match, --copy-of, value-of, etc options\n"
"in a single template. The effect of applying command line templates\n"
"can be illustrated with the following XSLT analogue\n\n"

"xml sel -t -c \"xpath0\" -m \"xpath1\" -m \"xpath2\" -v \"xpath3\" \\\n"
"        -t -m \"xpath4\" -c \"xpath5\"\n\n"

"is equivalent to applying the following XSLT\n\n"

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
"</xsl:template>\n"
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
    fprintf(o, select_usage_str);
    fprintf(o, more_info);
    fprintf(o, libxslt_more_info);
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
    ops->no_omit_decl = 0;
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
        else if (!strcmp(argv[i], "-T"))
        {
            ops->outText = 1;
        }
        else if (!strcmp(argv[i], "-R"))
        {
            ops->printRoot = 1;
        }
        else if (!strcmp(argv[i], "-I"))
        {
            ops->indent = 1;
        }
        else if (!strcmp(argv[i], "-D"))
        {
            ops->no_omit_decl = 1;
        }
        else if (!strcmp(argv[i], "--help"))
        {
            selUsage(argc, argv);
        }
        i++;
    }

    return i-1;
}

/**
 *  Prepare XSLT stylesheet based on command line options
 */
int
selPrepareXslt(char* xsl_buf, int *len, selOptionsPtr ops,
               int start, int argc, char **argv)
{
    int c, i, j, k, m, t;
    int templateEmpty = 1;
    
    xsl_buf[0] = 0;
    *len = 0;
    c = 0;

    c += sprintf(xsl_buf, "<?xml version=\"1.0\"?>\n");
    c += sprintf(xsl_buf + c,
      "<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n");

    if (ops->no_omit_decl) c += sprintf(xsl_buf + c, "<xsl:output omit-xml-declaration=\"no\"");
    else c += sprintf(xsl_buf + c, "<xsl:output omit-xml-declaration=\"yes\"");
    if (ops->indent) c += sprintf(xsl_buf + c, " indent=\"yes\"");
    else c += sprintf(xsl_buf + c, " indent=\"no\"");
    if (ops->outText) c += sprintf(xsl_buf + c, " method=\"text\"");
    c += sprintf(xsl_buf + c, "/>\n");

    c += sprintf(xsl_buf + c, "<xsl:param name=\"inputFile\">-</xsl:param>\n");

    c += sprintf(xsl_buf + c, "<xsl:template match=\"/\">\n");
    if (!ops->outText && ops->printRoot) c += sprintf(xsl_buf + c, "<xml-select>\n");

    t = 0;
    i = start;
    while(i < argc)
    {
        if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template"))
        {
            t++;
            c += sprintf(xsl_buf + c, "  <xsl:call-template name=\"t%d\"/>\n", t);
        }
        i++;
    }
    if (!ops->outText && ops->printRoot) c += sprintf(xsl_buf + c, "</xml-select>\n");
    c += sprintf(xsl_buf + c, "</xsl:template>\n");

    /*
     *  At least one -t option must be found
     */
    if (t == 0)
    {
        fprintf(stderr, "error in arguments:");
        fprintf(stderr, " none -t or --template options found\n");
        /*selUsage(argc, argv);*/
        exit(2);
    }
    
    t = 0;
    i = 2;
    while(i < argc)
    {
        if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template"))
        {
            templateEmpty = 1;
            t++;
            c += sprintf(xsl_buf + c, "<xsl:template name=\"t%d\">\n", t);

            i++;
            m = 0;
            while(i < (argc - 1))
            {
                if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--copy-of"))
                {
                    templateEmpty = 0;
                    for (j=0; j <= m; j++) c += sprintf(xsl_buf + c, "  ");
                    c += sprintf(xsl_buf + c, "<xsl:copy-of select=\"%s\"/>\n", argv[i+1]);
                    i++;
                }
                else if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--value-of"))
                {
                    templateEmpty = 0;
                    for (j=0; j <= m; j++) c += sprintf(xsl_buf + c, "  ");
                    c += sprintf(xsl_buf + c, "<xsl:value-of select=\"%s\"/>\n", argv[i+1]);
                    i++;
                }
                else if(!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
                {
                    templateEmpty = 0;
                    for (j=0; j <= m; j++) c += sprintf(xsl_buf + c, "  ");
                    c += sprintf(xsl_buf + c, "<xsl:value-of select=\"'%s'\"/>\n", argv[i+1]);
                    i++;
                }
                else if(!strcmp(argv[i], "-n") || !strcmp(argv[i], "--nl"))
                {
                    templateEmpty = 0;
                    for (j=0; j <= m; j++) c += sprintf(xsl_buf + c, "  ");
                    c += sprintf(xsl_buf + c, "<xsl:value-of select=\"'&#10;'\"/>\n");
                }
                else if(!strcmp(argv[i], "-m") || !strcmp(argv[i], "--match"))
                {
                    templateEmpty = 0;
                    for (j=0; j <= m; j++) c += sprintf(xsl_buf + c, "  ");
                    c += sprintf(xsl_buf + c, "<xsl:for-each select=\"%s\">\n", argv[i+1]);
                    m++;
                    i++;
                }
                else if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template"))
                {
                    i--;
                    break;
                }
                else
                {
                    if(argv[i][0] != '-')
                    {
                        break;
                    }
                }

                i++;
            }
/*
            if((i < argc) && (!strcmp(argv[i], "-n") || !strcmp(argv[i], "--nl")))
            {
                for (j=0; j <= m; j++) c += sprintf(xsl_buf + c, "  ");
                c += sprintf(xsl_buf + c, "<xsl:value-of select=\"'&#10;'\"/>\n");
            }
*/
            for (k=0; k<m; k++)
            {
                for (j=k; j<m; j++) c += sprintf(xsl_buf + c, "  ");
                c += sprintf(xsl_buf + c, "</xsl:for-each>\n");
            }

            if (templateEmpty)
            {
                fprintf(stderr, "error in arguments:");
                fprintf(stderr, " -t or --template option must be followed by");
                fprintf(stderr, " --match or other options\n");
                /*selUsage(argc, argv);*/
                exit(3);
            }
            
            c += sprintf(xsl_buf + c, "</xsl:template>\n");
        }

        if (i >= argc) break;
        if (argv[i][0] != '-') break;

        i++;
    }

    c += sprintf(xsl_buf + c, "</xsl:stylesheet>\n");
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
    int start, c, i, n;
    int nbparams;
  
    if (argc <= 2) selUsage(argc, argv);

    selInitOptions(&ops);
    xsltInitOptions(&xsltOps);
    start = selParseOptions(&ops, argc, argv);    

    c = sizeof(xsl_buf);    
    i = selPrepareXslt(xsl_buf, &c, &ops, start, argc, argv);
    
    if (ops.printXSLT) fprintf(stdout, "%s", xsl_buf);
    
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
            xmlDocPtr doc = xmlParseFile(argv[n]);
            xsltProcess(&xsltOps, doc, params, cur, argv[n]);
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
        xmlDocPtr doc = xmlParseFile("-");
        xsltProcess(&xsltOps, doc, params, cur, "-");
        /* xsltFreeStylesheet(cur); */
    }
        
    return 0;
}  

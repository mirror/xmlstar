/*  $Id: xml_format.c,v 1.7 2002/12/07 19:17:43 mgrouch Exp $  */

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

#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/xmlIO.h>
#include <libxml/HTMLtree.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xpointer.h>
#include <libxml/parserInternals.h>
#include <libxml/uri.h>

/*
 *   TODO:
 *        1. Attribute formatting options (as every attribute on a new line)
 *        2. exit values on errors
 */

typedef struct _foOptions {
    int indent;               /* Indent output */
    int indent_tab;           /* Indent output with tab */
    int indent_spaces;        /* Num spaces for indentation */
#ifdef LIBXML_HTML_ENABLED
    int html;                 /* inputs are in HTML format */
#endif
} foOptions;

typedef foOptions *foOptionsPtr;

static const char format_usage_str[] =
"XMLStarlet Toolkit: Format XML document(s)\n"
"Usage: xml fo [<options>] <xml-file>\n"
"where <options> are\n"
"   -t or --indent-tab          - indent output with tabulation\n"
"   -s or --indent-spaces <num> - indent output with <num> spaces\n"
"   -n or --noindent            - do not indent\n"
#ifdef LIBXML_HTML_ENABLED
"   --html                      - input is HTML\n"
#endif
"   -h or --help                - print help\n\n";

/**
 *  Print small help for command line options
 */
void
foUsage(int argc, char **argv)
{
    extern const char more_info[];
    FILE* o = stderr;
    fprintf(o, format_usage_str);
    fprintf(o, more_info);
    exit(1);
}

/**
 *  Initialize global command line options
 */
void
foInitOptions(foOptionsPtr ops)
{
    ops->indent = 1;
    ops->indent_tab = 0;
    ops->indent_spaces = 2;
#ifdef LIBXML_HTML_ENABLED
    ops->html = 0;
#endif
}

/**
 *  Initialize LibXML
 */
void
foInitLibXml(foOptionsPtr ops)
{
    /*
     * Initialize library memory
     */
    xmlInitMemory();

    LIBXML_TEST_VERSION

    /*
     * Store line numbers in the document tree
     */
    xmlLineNumbersDefault(1);

    xmlSubstituteEntitiesDefault(1);
    xmlKeepBlanksDefault(0);
    xmlPedanticParserDefault(0);
    
    xmlGetWarningsDefaultValue = 1;
    xmlDoValidityCheckingDefaultValue = 0;
    xmlLoadExtDtdDefaultValue = 0;

    xmlTreeIndentString = NULL;
    if (ops->indent)
    {
        xmlIndentTreeOutput = 1;
        if (ops->indent_spaces > 0)
        {
            int i;
            char *p;
            p = malloc(ops->indent_spaces + 1);
            xmlTreeIndentString = p;
            for (i=0; i<ops->indent_spaces; i++) p[i] = ' ';
            p[ops->indent_spaces] = 0;
        }
        if (ops->indent_tab) xmlTreeIndentString = strdup("\t");
    }
    else
        xmlIndentTreeOutput = 0;
}

/**
 *  Parse global command line options
 */
int
foParseOptions(foOptionsPtr ops, int argc, char **argv)
{
    int i;

    i = 2;
    while(i < argc)
    {
        if (!strcmp(argv[i], "--noindent") || !strcmp(argv[i], "-n"))
        {
            ops->indent = 0;
            i++;
        }
        else if (!strcmp(argv[i], "--indent-tab") || !strcmp(argv[i], "-t"))
        {
            ops->indent_tab = 1;
            i++;
        }
        else if (!strcmp(argv[i], "--indent-spaces") || !strcmp(argv[i], "-s"))
        {
            int value;
            i++;
            if (i >= argc) foUsage(argc, argv);
            if (sscanf(argv[i], "%d", &value) == 1)
            {
                if (value > 0) ops->indent_spaces = value;
            }
            else
            {
                foUsage(argc, argv);
            }
            ops->indent_tab = 0;
            i++;
        }
#ifdef LIBXML_HTML_ENABLED
        else if (!strcmp(argv[i], "--html"))
        {
            ops->html = 1;
            i++;
        }
#endif
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
        {
            foUsage(argc, argv);
        }
        else if (!strcmp(argv[i], "-"))
        {
            i++;
            break;
        }
        else if (argv[i][0] == '-')
        {
            foUsage(argc, argv);
        }
        else
        {
            i++;
            break;
        }
    }

    return i-1;
}

/**
 *  'process' xml document(s)
 */
int
foProcess(foOptionsPtr ops, int start, int argc, char **argv)
{
    int ret = 0;
    xmlDocPtr doc = NULL;
    char *fileName = "-";

    if ((start < argc) && (argv[start][0] != '-') &&
        strcmp(argv[start-1], "--indent-spaces") &&
        strcmp(argv[start-1], "-s"))
    {
        fileName = argv[start];   
    }
    
#ifdef LIBXML_HTML_ENABLED
    if (ops->html)
        doc = htmlParseFile(fileName, NULL);
    else
#endif
        doc = xmlParseFile(fileName);
    xmlSaveFormatFile("-", doc, 1);

    return ret;
}

/**
 *  Cleanup memory
 */
void
foCleanup()
{
    xmlCleanupParser();
    if (xmlTreeIndentString) free((char *)xmlTreeIndentString);
#if 0
    xmlMemoryDump();
#endif
}

/**
 *  This is the main function for 'format' option
 */
int
foMain(int argc, char **argv)
{
    int ret = 0;
    int start;
    static foOptions ops;

    if (argc <=2) foUsage(argc, argv);
    foInitOptions(&ops);
    start = foParseOptions(&ops, argc, argv);
    if (argc-start > 1) foUsage(argc, argv);
    foInitLibXml(&ops);
    ret = foProcess(&ops, start, argc, argv);
    foCleanup();
    
    return ret;
}

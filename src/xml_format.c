/*  $Id: xml_format.c,v 1.1 2002/12/02 04:52:32 mgrouch Exp $  */

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
   TODO:
 */

typedef struct _foOptions {
    int indent;               /* Indent output */
    int indent_tab;           /* Indent output with tab */
    int indent_spaces;        /* Num spaces for indentation */
} foOptions;

typedef foOptions *foOptionsPtr;

static const char format_usage_str[] =
"XMLStarlet Toolkit: Format XML document(s)\n"
"Usage: xml fo [<options>] <xml-file>\n"
"where <options> are\n"
"   --indent-tab              - indent output with tabulation\n"
"   --indent-spaces <num>     - indent output with <num> spaces\n"
"   --noindent                - do not indent\n\n";

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

    if (ops->indent)
    {
        xmlIndentTreeOutput = 1;
        xmlTreeIndentString = "  ";
        if (ops->indent_tab) xmlTreeIndentString = "\t";        
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
    while((i < argc) && (strcmp(argv[i], "-t")) && strcmp(argv[i], "--template"))
    {
        if (!strcmp(argv[i], "--noindent"))
        {
            ops->indent = 0;
        }
        if (!strcmp(argv[i], "--indent-tab"))
        {
            ops->indent_tab = 1;
        }
        else if (!strcmp(argv[i], "--help"))
        {
            foUsage(argc, argv);
        }
        i++;
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
    
    doc = xmlParseFile("-");

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

    foInitOptions(&ops);
    start = foParseOptions(&ops, argc, argv);
    foInitLibXml(&ops);
    ret = foProcess(&ops, start, argc, argv);
    foCleanup();
    
    return ret;
}

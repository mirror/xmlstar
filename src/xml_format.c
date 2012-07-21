/*  $Id: xml_format.c,v 1.25 2005/01/07 02:33:40 mgrouch Exp $  */

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

#include "xmlstar.h"

/*
 *  TODO:  1. Attribute formatting options (as every attribute on a new line)
 *         2. exit values on errors
 */

typedef struct _foOptions {
    int indent;               /* indent output */
    int indent_tab;           /* indent output with tab */
    int indent_spaces;        /* num spaces for indentation */
    int omit_decl;            /* omit xml declaration */
    int recovery;             /* try to recover what is parsable */
    int dropdtd;              /* remove the DOCTYPE of the input docs */
    int options;              /* global parsing flags */ 
#ifdef LIBXML_HTML_ENABLED
    int html;                 /* inputs are in HTML format */
#endif
    int quiet;                 /* quiet mode */
} foOptions;

typedef foOptions *foOptionsPtr;

const char *encoding = NULL;
static char *spaces = NULL;

/**
 *  Print small help for command line options
 */
void
foUsage(int status)
{
    extern void fprint_format_usage(FILE* o, const char* argv0);
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprint_format_usage(o, get_arg(ARG0));
    fprintf(o, "%s", more_info);
    exit(status);
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
    ops->omit_decl = 0;
    ops->recovery = 0;
    ops->dropdtd = 0;
    ops->options = XML_PARSE_NONET;
#ifdef LIBXML_HTML_ENABLED
    ops->html = 0;
#endif
    ops->quiet = 0;
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
        if (ops->indent_tab) 
        {
            xmlTreeIndentString = "\t";
        }
        else if (ops->indent_spaces > 0)
        {
            spaces = xmlMalloc(ops->indent_spaces + 1);
            xmlTreeIndentString = spaces;
            memset(spaces, ' ', ops->indent_spaces);
            spaces[ops->indent_spaces] = '\0';
        }
    }
    else
        xmlIndentTreeOutput = 0;
}

/**
 *  Parse global command line options
 */
void
foParseOptions(foOptionsPtr ops)
{
    for (;;) {
        const char* arg = get_arg(OPTION_NEXT);
        if (!arg) break;

        if (strcmp(arg, "--noindent") == 0 || strcmp(arg, "-n") == 0) {
            ops->indent = 0;
        } else if (strcmp(arg, "--encode") == 0 || strcmp(arg, "-e") == 0) {
            encoding = get_arg(ARG_NEXT);
        } else if (strcmp(arg, "--indent-tab") == 0 || strcmp(arg, "-t") == 0) {
            ops->indent_tab = 1;
        } else if (strcmp(arg, "--omit-decl") == 0 || strcmp(arg, "-o") == 0) {
            ops->omit_decl = 1;
        } else if (strcmp(arg, "--dropdtd") == 0 || strcmp(arg, "-D") == 0) {
            ops->dropdtd = 1;
        } else if (strcmp(arg, "--recover") == 0 || strcmp(arg, "-R") == 0) {
            ops->recovery = 1;
            ops->options |= XML_PARSE_RECOVER;
        } else if (strcmp(arg, "--nocdata") == 0 || strcmp(arg, "-C") == 0) {
            ops->options |= XML_PARSE_NOCDATA;
        } else if (strcmp(arg, "--nsclean") == 0 || strcmp(arg, "-N") == 0) {
            ops->options |= XML_PARSE_NSCLEAN;
        } else if (strcmp(arg, "--indent-spaces") == 0 || strcmp(arg, "-s") == 0) {
            int value, got_value = 0;
            arg = get_arg(ARG_NEXT);
            if (arg) got_value = (sscanf(arg, "%d", &value) == 1);

            if (got_value) {
                ops->indent_spaces = value;
            } else {
                fprintf(stderr, "--indent-spaces must be followed by <num>\n");
                exit(EXIT_BAD_ARGS);
            }
            ops->indent_tab = 0;
        } else if (strcmp(arg, "--quiet") == 0 || strcmp(arg, "-Q") == 0) {
            ops->quiet = 1;
#ifdef LIBXML_HTML_ENABLED
        } else if (strcmp(arg, "--html") == 0 || strcmp(arg, "-H") == 0) {
            ops->html = 1;
#endif
        } else if (strcmp(arg, "--net") == 0) {
            ops->options &= ~XML_PARSE_NONET;
        } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            foUsage(EXIT_SUCCESS);
        } else {
            fprintf(stderr, "unrecognized option %s\n", arg);
            exit(EXIT_BAD_ARGS);
        }
    }
}

void my_error_func(void* ctx, const char * msg, ...) {
  /* do nothing */
}

void my_structured_error_func(void * userData, xmlErrorPtr error) {
  /* do nothing */
}

/**
 *  'process' xml document(s)
 */
int
foProcess(foOptionsPtr ops)
{
    int ret = 0;
    xmlDocPtr doc = NULL;
    char *fileName = "-";

    if (get_arg(ARG_PEEK))
        fileName = get_arg(ARG_NEXT);


    if (ops->recovery) {
        doc = xmlRecoverFile(fileName);
    } else if (ops->quiet) {
        xmlSetGenericErrorFunc(NULL, my_error_func);
        xmlSetStructuredErrorFunc(NULL, my_structured_error_func);
    }

#ifdef LIBXML_HTML_ENABLED
    if (ops->html)
    {
        doc = htmlReadFile(fileName, NULL, ops->options);
    }
    else
#endif
        doc = xmlReadFile(fileName, NULL, ops->options);

    if (doc == NULL)
    {
        /*fprintf(stderr, "%s:: error: XML parse error\n", fileName);*/
        return 2;
    }

    /*
     * Remove DOCTYPE nodes
     */
    if (ops->dropdtd) {
        xmlDtdPtr dtd;

        dtd = xmlGetIntSubset(doc);
        if (dtd != NULL) {
            xmlUnlinkNode((xmlNodePtr)dtd);
            xmlFreeDtd(dtd);
        }
    }
 
    if (!ops->omit_decl)
    {
        if (encoding != NULL)
        {
            xmlSaveFormatFileEnc("-", doc, encoding, 1);
        }
        else
        {
            xmlSaveFormatFile("-", doc, 1);
        }
    }
    else
    {
        int format = 1;
        xmlOutputBufferPtr buf = NULL;
        xmlCharEncodingHandlerPtr handler = NULL;
        buf = xmlOutputBufferCreateFile(stdout, handler);

        if (doc->children != NULL)
        {
            xmlNodePtr child = doc->children;
            while (child != NULL)
            {
                xmlNodeDumpOutput(buf, doc, child, 0, format, encoding);
                xmlOutputBufferWriteString(buf, "\n");
                child = child->next;
            }
        }
        ret = xmlOutputBufferClose(buf);
    }
    
    xmlFreeDoc(doc);
    return ret;
}

/**
 *  Cleanup memory
 */
void
foCleanup()
{
    free(spaces);
    spaces = NULL;
    xmlCleanupParser();
#if 0
    xmlMemoryDump();
#endif
}

/**
 *  This is the main function for 'format' option
 */
int
foMain(void)
{
    int ret = 0;
    static foOptions ops;

    if (!get_arg(ARG_PEEK)) foUsage(EXIT_BAD_ARGS);
    foInitOptions(&ops);
    foParseOptions(&ops);
    foInitLibXml(&ops);
    ret = foProcess(&ops);
    foCleanup();
    
    return ret;
}

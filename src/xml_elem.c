/*  $Id: xml_elem.c,v 1.14 2003/12/17 06:26:01 mgrouch Exp $  */

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

#include <libxml/parser.h>
#include <stdlib.h>
#include <string.h>

#include "binsert.h"

#include "strdup.h"

/* TODO:

   1. Option for equivalent of
      xml el <xml-file> | sort | uniq
      Maintain sorted array and do binary insertion

   2. Option to display this only for nodes matching
      an XPATH expression

      -p <xpath>

      so it will be able to deal with subtrees as well

   3. Use stdin if input file is missing         

*/

typedef struct _elOptions {
    int show_attr;            /* show attributes */
    int show_attr_and_val;    /* show attributes and values */
    int sort_uniq;            /* do sort and uniq on output */
} etOptions;


static const char elem_usage_str[] =
"XMLStarlet Toolkit: Display element structure of XML document\n"
"Usage: xml el [<options>] <xml-file>\n"
"where\n"
"   <xml-file> - input XML document file name (stdin is used if missing)\n"
"   <options>:\n"
"   -a  - show attributes as well\n"
"   -v  - show attributes and their values\n"
"   -u  - print out sorted unique lines\n"
"\n";

static xmlSAXHandler xmlSAX_handler;
static etOptions elOps;
static SortedArray sorted = NULL;

#define LINE_BUF_SZ  4*1024

static char curXPath[LINE_BUF_SZ]; /* TODO: do not hardcode size */

/**
 *  Display usage syntax
 */
void
elUsage(int argc, char **argv)
{
    extern const char more_info[];
    FILE* o = stderr;
    fprintf(o, elem_usage_str);
    fprintf(o, more_info);
    exit(1);
}

/**
 *  onStartElement SAX parser callback
 */
void elStartElement(void *user_data, const xmlChar *name, const xmlChar **attrs)
{
    char *tmpXPath = NULL;
    
    if (*curXPath != (char)0) strcat(curXPath, "/");
    strcat(curXPath, (char*) name);

    if (elOps.show_attr)
    {
        const xmlChar **p = attrs;

        fprintf(stdout, "%s\n", curXPath);
        while (p && *p)
        {
            fprintf(stdout, "%s/@%s\n", curXPath, *p);
            p += 2;
        }
    }
    else if (elOps.show_attr_and_val)
    {
        const xmlChar **p = attrs;
        
        fprintf(stdout, "%s", curXPath);
        if (attrs) fprintf(stdout, "[");
        while (p && *p)
        {
            if (p != attrs) fprintf(stdout, " and ");
            fprintf(stdout, "@%s=\'%s\'", *p, *(p+1));
            p += 2;
        }
        if (attrs) fprintf(stdout, "]");
        fprintf(stdout, "\n");
    }
    else
    {
        if (elOps.sort_uniq)
        {
            int idx;
            tmpXPath = strdup(curXPath);
            idx = array_binary_insert(sorted, tmpXPath);
            if (idx < 0)
            {
                free(tmpXPath);
                tmpXPath = NULL;
            }
        }
        else fprintf(stdout, "%s\n", curXPath);
    }
}

/**
 *  onEndElement SAX parser callback
 */
void elEndElement(void *user_data, const xmlChar *name)
{
    *(curXPath + strlen(curXPath) - strlen((char*) name) - 1) = '\0';
}

/**
 *  run SAX parser on XML file
 */
int
parse_xml_file(const char *filename)
{ 
    int ret;

    xmlInitParser();

    memset(&xmlSAX_handler, 0, sizeof(xmlSAX_handler));

    xmlSAX_handler.startElement = elStartElement;
    xmlSAX_handler.endElement = elEndElement;

    ret = xmlSAXUserParseFile(&xmlSAX_handler, NULL, filename);
    xmlCleanupParser();

    if (ret < 0)
    {
        return ret;
    }
    else
        return ret;
}

/**
 *  Initialize options values
 */
void
elInitOptions(etOptions *ops)
{
    ops->show_attr = 0;  
    ops->show_attr_and_val = 0;
    ops->sort_uniq = 0;
}

/**
 *  This is the main function for 'el' option
 */
int
elMain(int argc, char **argv)
{
    int errorno = 0;
    char* inp_file = "-";

    if (argc <= 1) elUsage(argc, argv);

    elInitOptions(&elOps);
    
    /* TODO: more options to be added */
    if (argc == 2)
        errorno = parse_xml_file("-");  
    else
    {
        if (!strcmp(argv[2], "--help") || !strcmp(argv[2], "-h") ||
            !strcmp(argv[2], "-?") || !strcmp(argv[2], "-Z"))
        {
            elUsage(argc, argv);
        }
        else if (!strcmp(argv[2], "-a"))
        {
            elOps.show_attr = 1;
            if (argc >= 4) inp_file = argv[3];
            errorno = parse_xml_file(inp_file);
        }
        else if (!strcmp(argv[2], "-v"))
        {
            elOps.show_attr_and_val = 1;
            if (argc >= 4) inp_file = argv[3];
            errorno = parse_xml_file(inp_file);
        }
        else if (!strcmp(argv[2], "-u"))
        {
            elOps.sort_uniq = 1;
            if (argc >= 4) inp_file = argv[3];
            sorted = array_create();
            errorno = parse_xml_file(inp_file);
        }
        else if (argv[2][0] != '-')
        {
            errorno = parse_xml_file(argv[2]);
        }
        else
            elUsage(argc, argv);
    }

    if (sorted)
    {
        int i;

        /* printf("array len: %d\n", array_len(sorted)); */

        for (i=0; i < array_len(sorted); i++)
        {
            char* item = array_item(sorted, i);
            printf("%s\n", item);
            free(item);
        }
        
        array_free(sorted);
        sorted = NULL;
    }  

    return errorno;
}

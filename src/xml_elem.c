/*  $Id: xml_elem.c,v 1.23 2004/11/21 23:40:40 mgrouch Exp $  */

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

#include <libxml/parser.h>
#include <libxml/xmlstring.h>
#include <stdlib.h>
#include <string.h>

#include "xmlstar.h"
#include "binsert.h"
#include "escape.h"

/* TODO:

   2. Option to display this only for nodes matching
      an XPATH expression

      -p <xpath>

      so it will be able to deal with subtrees as well

*/

typedef struct _elOptions {
    int show_attr;            /* show attributes */
    int show_attr_and_val;    /* show attributes and values */
    int sort_uniq;            /* do sort and uniq on output */
    int check_depth;          /* limit depth */
} elOptions;


static const char elem_usage_str[] =
"XMLStarlet Toolkit: Display element structure of XML document\n"
"Usage: %s el [<options>] <xml-file>\n"
"where\n"
"  <xml-file> - input XML document file name (stdin is used if missing)\n"
"  <options> is one of:\n"
"  -a    - show attributes as well\n"
"  -v    - show attributes and their values\n"
"  -u    - print out sorted unique lines\n"
"  -d<n> - print out sorted unique lines up to depth <n>\n" 
"\n";

static xmlSAXHandler xmlSAX_handler;
static elOptions elOps;
static SortedArray sorted = NULL;

#define LINE_BUF_SZ  4*1024

static xmlChar *curXPath = NULL;
static int depth = 0;

/**
 *  Display usage syntax
 */
void
elUsage(int argc, char **argv, exit_status status)
{
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprintf(o, elem_usage_str, argv[0]);
    fprintf(o, "%s", more_info);
    exit(status);
}

/**
 *  onStartElement SAX parser callback
 */
void elStartElement(void *user_data, const xmlChar *name, const xmlChar **attrs)
{
    if (depth > 0) curXPath = xmlStrcat(curXPath, BAD_CAST "/");
    curXPath = xmlStrcat(curXPath, name);
    depth++;

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
        xmlChar *xml_str = NULL;
        
        fprintf(stdout, "%s", curXPath);
        if (attrs) fprintf(stdout, "[");
        while (p && *p)
        {
            if (p != attrs) fprintf(stdout, " and ");
            
            /*xml_str = xml_C11NNormalizeAttr((const xmlChar *) *(p+1));*/
            xml_str = xmlStrdup((const xmlChar *) *(p+1));
            if (xmlStrchr(xml_str, '\''))
            {
                fprintf(stdout, "@%s=&quot;%s&quot;", *p, xml_str);
            }
            else
            {
                fprintf(stdout, "@%s=\'%s\'", *p, xml_str);
            }
            p += 2;
            xmlFree(xml_str);
        }
        if (attrs) fprintf(stdout, "]");
        fprintf(stdout, "\n");
    }
    else
    {
        if (elOps.sort_uniq)
        {
            if ((elOps.check_depth == 0) || (elOps.check_depth != 0 && depth <= elOps.check_depth))
            { 
                int idx;
                xmlChar *tmpXPath = xmlStrdup(curXPath);
    
                idx = array_binary_insert(sorted, tmpXPath);
                if (idx < 0)
                {
                   free(tmpXPath);
                   tmpXPath = NULL;
                }
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
    int xlen = xmlStrlen(curXPath);
    int nlen = xmlStrlen(name);
    *(curXPath + xlen - nlen - (xlen == nlen?0:1)) = '\0';
    depth--;
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
    xmlFree(curXPath);
    return ret;
}

/**
 *  Initialize options values
 */
void
elInitOptions(elOptions *ops)
{
    ops->show_attr = 0;  
    ops->show_attr_and_val = 0;
    ops->sort_uniq = 0;
    ops->check_depth = 0; 
}

/**
 *  This is the main function for 'el' option
 */
int
elMain(int argc, char **argv)
{
    int errorno = 0;
    char* inp_file = "-";

    if (argc <= 1) elUsage(argc, argv, EXIT_BAD_ARGS);

    elInitOptions(&elOps);

    if (argc == 2)
        errorno = parse_xml_file("-");  
    else
    {
        if (!strcmp(argv[2], "--help") || !strcmp(argv[2], "-h") ||
            !strcmp(argv[2], "-?") || !strcmp(argv[2], "-Z"))
        {
            elUsage(argc, argv, EXIT_SUCCESS);
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
        else if (!strncmp(argv[2], "-d", 2)) 
        { 
            elOps.check_depth = atoi(argv[2]+2); 
            /* printf("Checking depth (%d)\n", elOps.check_depth); */ 
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
            elUsage(argc, argv, EXIT_BAD_ARGS);
    }

    if (sorted)
    {
        int i;

        /* printf("array len: %d\n", array_len(sorted)); */

        for (i=0; i < array_len(sorted); i++)
        {
            xmlChar* item = array_item(sorted, i);
            printf("%s\n", item);
            free(item);
        }
        
        array_free(sorted);
        sorted = NULL;
    }  

    return errorno;
}


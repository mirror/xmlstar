/*  $Id: xml_elem.c,v 1.1 2003/04/23 02:43:36 mgrouch Exp $  */

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
#include <string.h>

/* TODO:

   1. Option for equivalent of
      xml el <xml-file> | sort | uniq
      Maintain sorted array and do binary insertion

   2. Option to display attributes values as well
      Ex:
      /xml/elem[attr1="val1" and attr2="val2"]

   3. Use stdin if input file is missing         

*/

static const char elem_usage_str[] =
"XMLStarlet Toolkit: Display elements structure of XML document\n"
"Usage: xml el [<options>] <xml-file>\n"
"where\n"
"   <xml-file> - input XML document file name (stdin is used if missing)\n"
"\n";

static xmlSAXHandler xmlSAX_handler;
static char curXPath[4*1024]; /* TODO: do not hardcode */

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
    if (*curXPath != (char)0) strcat(curXPath, "/");
    strcat(curXPath, (char*) name);
}

/**
 *  onEndElement SAX parser callback
 */
void elEndElement(void *user_data, const xmlChar *name)
{
    fprintf(stdout, "%s\n", curXPath);
    *(curXPath + strlen(curXPath) - strlen((char*) name) - 1) = '\0';
}

/**
 *  run SAX parser on XML file
 */
int
parse_xml_file(const char *filename)
{ 
    int ret;

    memset(&xmlSAX_handler, 0, sizeof(xmlSAX_handler));

    xmlSAX_handler.startElement = elStartElement;
    xmlSAX_handler.endElement = elEndElement;
   
    if ((ret = xmlSAXUserParseFile(&xmlSAX_handler, NULL, filename)) < 0)
    {
        return ret;
    }
    else
        return ret;
}

/**
 *  This is the main function for 'el' option
 */
int
elMain(int argc, char **argv)
{
    int errorno = 0;

    if (argc <= 2) elUsage(argc, argv);

    errorno = parse_xml_file(argv[2]);  /* TODO: more options will be added */
    
    return errorno;
}

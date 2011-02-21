/*  $Id: xml_escape.c,v 1.11 2004/11/21 23:40:40 mgrouch Exp $  */

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

#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/xmlIO.h>
#include <libxml/parserInternals.h>

#include "xmlstar.h"
#include "escape.h"

#define INSZ 4*1024

/*
 *  TODO:  1. stdin input
 *         2. exit values on errors
 */

static const char escape_usage_str[] =
"XMLStarlet Toolkit: %s special XML characters\n"
"Usage: %s %s [<options>] [<string>]\n"
"where <options> are\n"
"  --help      - print usage\n"
"  (TODO: more to be added in future)\n"
"if <string> is missing stdin is used instead.\n"
"\n";

/**
 *  Print small help for command line options
 */
void
escUsage(int argc, char **argv, int escape, exit_status status)
{
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprintf(o, escape_usage_str, escape?"Escape":"Unescape", argv[0], escape?"esc":"unesc");
    fprintf(o, "%s", more_info);
    exit(status);
}

struct xmlPredefinedChar {
    const char *name;
    char        value;
    int         name_len;
};

static struct xmlPredefinedChar xmlPredefinedCharValues[] = {
    { "lt", '<', 2 },
    { "gt", '>', 2 },
    { "apos", '\'', 4 },
    { "quot", '\"', 4 },
    { "amp", '&', 3 },
    { NULL, '\0', 0 }
};

/*
 * Macro used to grow the current buffer.
 */
#define grow_BufferReentrant() { 					\
    buffer_size *= 2;   						\
    buffer = (xmlChar *)						\
    		xmlRealloc(buffer, buffer_size * sizeof(xmlChar));	\
    if (buffer == NULL) {       					\
	fprintf(stderr, "growing buffer error");			\
	abort();							\
    }									\
}

/** 
 * xml_C11NNormalizeString:
 * @input: the input string
 * @mode:  the normalization mode (attribute, comment, PI or text)
 *
 * Converts a string to a canonical (normalized) format. The code is stolen
 * from xmlEncodeEntitiesReentrant(). Added normalization of \x09, \x0a, \x0A
 * and the @mode parameter
 *
 * Returns a normalized string (caller is responsible for calling xmlFree())
 * or NULL if an error occurs
 */
xmlChar *
xml_C11NNormalizeString(const xmlChar * input,
                         xml_C14NNormalizationMode mode)
{
    const xmlChar *cur = input;
    xmlChar *buffer = NULL;
    xmlChar *out = NULL;
    int buffer_size = 0;

    if (input == NULL)
        return (NULL);

    /*
     * allocate an translation buffer.
     */
    buffer_size = 1000;
    buffer = (xmlChar *) xmlMallocAtomic(buffer_size * sizeof(xmlChar));
    if (buffer == NULL) {
        fprintf(stderr, "allocating buffer error");
        abort();
    }
    out = buffer;

    while (*cur != '\0') {
        if ((out - buffer) > (buffer_size - 10)) {
            int indx = out - buffer;

            grow_BufferReentrant();
            out = &buffer[indx];
        }

        if ((*cur == '<') && ((mode == XML_C14N_NORMALIZE_ATTR) ||
                              (mode == XML_C14N_NORMALIZE_TEXT))) {
            *out++ = '&';
            *out++ = 'l';
            *out++ = 't';
            *out++ = ';';
        } else if ((*cur == '>') && (mode == XML_C14N_NORMALIZE_TEXT)) {
            *out++ = '&';
            *out++ = 'g';
            *out++ = 't';
            *out++ = ';';
        } else if ((*cur == '&') && ((mode == XML_C14N_NORMALIZE_ATTR) ||
                                     (mode == XML_C14N_NORMALIZE_TEXT))) {
            *out++ = '&';
            *out++ = 'a';
            *out++ = 'm';
            *out++ = 'p';
            *out++ = ';';
        } else if ((*cur == '"') && (mode == XML_C14N_NORMALIZE_ATTR)) {
            *out++ = '&';
            *out++ = 'q';
            *out++ = 'u';
            *out++ = 'o';
            *out++ = 't';
            *out++ = ';';
        } else if ((*cur == '\x09') && (mode == XML_C14N_NORMALIZE_ATTR)) {
            *out++ = '&';
            *out++ = '#';
            *out++ = 'x';
            *out++ = '9';
            *out++ = ';';
        } else if ((*cur == '\x0A') && (mode == XML_C14N_NORMALIZE_ATTR)) {
            *out++ = '&';
            *out++ = '#';
            *out++ = 'x';
            *out++ = 'A';
            *out++ = ';';
        } else if ((*cur == '\x0D') && ((mode == XML_C14N_NORMALIZE_ATTR) ||
                                        (mode == XML_C14N_NORMALIZE_TEXT) ||
                                        (mode == XML_C14N_NORMALIZE_COMMENT) ||
                                        (mode == XML_C14N_NORMALIZE_PI))) {
            *out++ = '&';
            *out++ = '#';
            *out++ = 'x';
            *out++ = 'D';
            *out++ = ';';
        } else {
            /*
             * Works because on UTF-8, all extended sequences cannot
             * result in bytes in the ASCII range.
             */
            *out++ = *cur;
        }
        cur++;
    }
    *out++ = 0;
    return (buffer);
}

/* TODO: CHECK THIS PROCEDURE IT'S PROB FULL OF BUGS */
char *
xml_unescape(char* str)
{
   char *p = str, *p2 = NULL;
   char *ret = NULL;

   ret = (char*) xmlCharStrdup(str);
   p2 = ret;
   
   while(*p)
   {
      if (*p == '&')
      {
         struct xmlPredefinedChar *pair = xmlPredefinedCharValues;

         p++;
         if (*p == '\0') break;

         
         if (*p == '#')
         {
            int num;
            p++;
            if (*p == '\0') break;
            num = atoi(p);

            while((*p >= '0') && (*p <= '9')) p++;

            if (*p == ';')
            {
               *p2 = (char) num;
               p2++;
               p++;
            }
            continue;
         }
         else         
         {
            while(pair->name)
            {
               if (!strncmp(p, pair->name, pair->name_len))
               {
                  if (*(p+pair->name_len) == ';')
                  {
                     *p2 = pair->value;
                     p2++;
                     p += (pair->name_len + 1);
                     break;
                  }
               }
               pair++;
            }
            continue;
         }
      }

      *p2 = *p;
      p2++;
      p++;
   }

   *p2 = '\0';
   
   return ret;   
}

/**
 *  This is the main function for 'escape/unescape' options
 */
int
escMain(int argc, char **argv, int escape)
{
    int ret = 0;
    int readStdIn = 0;
        
    char* inp = NULL;
    xmlChar* outBuf = NULL;
    
    if (argc < 2) escUsage(argc, argv, escape, EXIT_BAD_ARGS);

    inp = argv[2];

    if (argc > 2)
    {
        if (!strcmp(argv[2], "--help") || !strcmp(argv[2], "-h") ||
           !strcmp(argv[2], "-?") || !strcmp(argv[2], "-Z"))
            escUsage(argc, argv, escape, EXIT_SUCCESS);
        if (!strcmp(argv[2], "-")) readStdIn = 1;
    }
    else
    {
        readStdIn = 1;        
    }

    if (readStdIn)
    {
       static char line[INSZ];

       while (!feof(stdin))
       {
           if (fgets(line, INSZ - 1, stdin))
           {
               if (escape)
               {
                   outBuf = xmlEncodeEntitiesReentrant(NULL, (xmlChar*) line);
                   if (outBuf)
                   {
                       fprintf(stdout, "%s", outBuf);
                       xmlFree(outBuf);
                   }
               }
               else
               {
                   outBuf = (xmlChar*) xml_unescape(line);
                   if (outBuf)
                   {
                       fprintf(stdout, "%s", outBuf);
                       free(outBuf);
                   }
               }
           }
       }
       
       return ret;
    }
    
    if (escape)
    {
        outBuf = xmlEncodeEntitiesReentrant(NULL, (xmlChar*) inp);
        if (outBuf)
        {
            fprintf(stdout, "%s\n", outBuf);
            xmlFree(outBuf);
        }
    }
    else
    {
        outBuf = (xmlChar*) xml_unescape(inp);
        if (outBuf)
        {
            fprintf(stdout, "%s\n", outBuf);
            free(outBuf);
        }
    }
        
    return ret;
}

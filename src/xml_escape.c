/*  $Id: xml_escape.c,v 1.5 2003/12/17 06:26:01 mgrouch Exp $  */

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

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/xmlIO.h>
#include <libxml/parserInternals.h>

#include "strdup.h" 

#define INSZ 4*1024

/*
 *  TODO:  1. stdin input
 *         2. exit values on errors
 */

static const char escape_usage_str[] =
"XMLStarlet Toolkit: %s special XML characters\n"
"Usage: xml %s [<options>] [<string>]\n"
"where <options> are\n"
"   --help      - print usage\n"
"   (TODO: more to be added in future)\n"
"if <string> is missing stdin is used instead.\n"
"\n";

/**
 *  Print small help for command line options
 */
void
escUsage(int argc, char **argv, int escape)
{
    extern const char more_info[];
    FILE* o = stderr;
    fprintf(o, escape_usage_str, escape?"Escape":"Unescape", escape?"esc":"unesc");
    fprintf(o, more_info);
    exit(1);
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

/* TODO: CHECK THIS PROCEDURE IT'S PROB FULL OF BUGS */
char *
xml_unescape(char* str)
{
   char *p = str, *p2 = NULL;
   char *ret = NULL;

   ret = strdup(str);
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
    
    if (argc < 2) escUsage(argc, argv, escape);

    inp = argv[2];

    if (argc > 2)
    {
        if (!strcmp(argv[2], "--help") || !strcmp(argv[2], "-h") ||
           !strcmp(argv[2], "-?") || !strcmp(argv[2], "-Z")) escUsage(argc, argv, escape);
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

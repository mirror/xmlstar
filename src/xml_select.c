/*  $Id: xml_select.c,v 1.10 2002/11/16 02:37:25 mgrouch Exp $  */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/xmlIO.h>
#include <libxml/HTMLtree.h>
#include <libxml/xinclude.h>
#include <libxml/parserInternals.h>
#include <libxml/uri.h>

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>
#if 0
#include <libxslt/security.h>
#endif


/*
 *  TODO:
 *
 *   1. How about sorting ?
 *   2.  -m  ->   -f  --for-each
 *       -p  ->   -c  --copy-of
 *           ->   -v  --value-of
 *       -s  ->   -o  --output
 *       -o  ->   -s  --sort
 */

#define MAX_XSL_BUF  256*1014

char xsl_buf[MAX_XSL_BUF];

void select_usage(int argc, char **argv)
{
    FILE* o = stderr;

    fprintf(o, "XMLStarlet Toolkit: Select from XML document(s)\n");
    fprintf(o, "Usage: xml sel {<template>} {<xml-file>}\n");
    fprintf(o, "where\n");
    fprintf(o, "  <xml-file> - input XML document file name (stdin is used if missing)\n");
    fprintf(o, "  <template> - template for querying XL document with following syntax:\n\n");

    fprintf(o, "Syntax for templates: -t|--template <options>\n");
    fprintf(o, "where <options>\n");
    fprintf(o, "  -c or --copy-of <xpath>  - print copy of XPATH expression\n");
    fprintf(o, "  -v or --value-of <xpath> - print value of XPATH expression\n");
    fprintf(o, "  -o or --output <order>   - print string literal \n");
    fprintf(o, "  -n or --nl              - print new line\n");
    fprintf(o, "  -s or --sort <order> - sort in order (used after -m)\n");
    fprintf(o, "  -m or --match <xpath>   - match XPATH expression\n");
    fprintf(o, "There can be multiple --match and --print options in a single template\n");
    fprintf(o, "Effect of applying command line templates can be illustrated with the following XSLT analogue\n\n");

    fprintf(o, "xml sel -t -c \"xpath0\" -m \"xpath1\" -m \"xpath2\" -v \"xpath3\" \\\n");
    fprintf(o, "        -t -m \"xpath4\" -c \"xpath5\"\n\n");

    fprintf(o, "is equivalent to applying the following XSLT\n\n");

    fprintf(o, "<?xml version=\"1.0\"?>\n");
    fprintf(o, "<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n");
    fprintf(o, "<xsl:template match=\"/\">\n");
    fprintf(o, "  <xsl:call-template name=\"t1\"/>\n");
    fprintf(o, "  <xsl:call-template name=\"t2\"/>\n");
    fprintf(o, "</xsl:template>\n");
    fprintf(o, "<xsl:template name=\"t1\">\n");
    fprintf(o, "  <xsl:copy-of select=\"xpath0\"/>\n");
    fprintf(o, "  <xsl:for-each select=\"xpath1\">\n");
    fprintf(o, "    <xsl:for-each select=\"xpath2\">\n");
    fprintf(o, "      <xsl:value-of select=\"xpath3\"/>\n");
    fprintf(o, "    </xsl:for-each>\n");
    fprintf(o, "  </xsl:for-each>\n");
    fprintf(o, "</xsl:template>\n");
    fprintf(o, "<xsl:template name=\"t2\">\n");
    fprintf(o, "  <xsl:for-each select=\"xpath4\">\n");
    fprintf(o, "    <xsl:copy-of select=\"xpath5\"/>\n");
    fprintf(o, "  </xsl:for-each>\n");
    fprintf(o, "</xsl:template>\n");
    fprintf(o, "</xsl:stylesheet>\n\n");  
        
    fprintf(o, "XMLStarlet is a command line toolkit to query/edit/check/transform\n");
    fprintf(o, "XML documents (for more information see http://xmlstar.sourceforge.net/)\n\n");

    fprintf(o, "Current implementation uses libxslt from GNOME codebase as XSLT processor\n");
    fprintf(o, "(see http://xmlsoft.org/ for more details)\n");

    exit(1);
}

/*
 *
 *  Use this script for testing (for now)
 
#! /bin/bash

./xml sel ${@+"$@"} >./sel.xsl
./xml tr sel.xsl in.xml

 *
 */


static int printXSLT = 0;
static int out_text = 0;

int xml_select(int argc, char **argv)
{
    int c, i, j, k, m, t;
  
    if (argc <= 2) select_usage(argc, argv);

#if 0
    fprintf(stderr, "SELECT\n");
    fprintf(stderr, "Sample: ./xml sel -t -p \"'--------'\" -m \"/xml/*\" -p \"position()\" -t -p \"count(/xml/*)\" -p \"'------FINISH-----'\"\n");
#endif


    /*
     *   Parse global options
     */
     
    i = 2;
    while((i < argc) && (strcmp(argv[i], "-t")) && strcmp(argv[i], "--template"))
    {
        if (!strcmp(argv[i], "-C"))
        {
            printXSLT = 1;
        }
        else if (!strcmp(argv[i], "-T"))
        {
            out_text = 1;
        }
        i++;  
    }


    xsl_buf[0] = 0;
    c = 0;
    
    c += sprintf(xsl_buf, "<?xml version=\"1.0\"?>\n");
    c += sprintf(xsl_buf + c, "<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n");
    if (out_text) c += sprintf(xsl_buf + c, "<xsl:output method=\"text\"/>\n");

    c += sprintf(xsl_buf + c, "<xsl:template match=\"/\">\n");
    if (!out_text) c += sprintf(xsl_buf + c, "<xml-select>\n");
    t = 0;
    i = 2;
    while(i < argc)
    {
        if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template"))
        {
            t++;
            c += sprintf(xsl_buf + c, "  <xsl:call-template name=\"t%d\"/>\n", t);
        }
        i++;  
    }
    if (!out_text) c += sprintf(xsl_buf + c, "</xml-select>\n");
    c += sprintf(xsl_buf + c, "</xsl:template>\n");
   
    t = 0;
    i = 2;
    while(i < argc)
    {
        if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template"))
        {
            t++;
            c += sprintf(xsl_buf + c, "<xsl:template name=\"t%d\">\n", t);

            i++;
            m = 0;
            while(i < (argc - 1))
            {
                if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--copy-of"))
                {
                    for (j=0; j <= m; j++) c += sprintf(xsl_buf + c, "  ");
                    c += sprintf(xsl_buf + c, "<xsl:copy-of select=\"%s\"/>\n", argv[i+1]);
                    i++;
                }
                else if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--value-of"))
                {
                    for (j=0; j <= m; j++) c += sprintf(xsl_buf + c, "  ");
                    c += sprintf(xsl_buf + c, "<xsl:value-of select=\"%s\"/>\n", argv[i+1]);
                    i++;
                }
                else if(!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output"))
                {
                    for (j=0; j <= m; j++) c += sprintf(xsl_buf + c, "  ");
                    c += sprintf(xsl_buf + c, "<xsl:value-of select=\"'%s'\"/>\n", argv[i+1]);
                    i++;
                }
                else if(!strcmp(argv[i], "-n") || !strcmp(argv[i], "--nl"))
                {
                    for (j=0; j <= m; j++) c += sprintf(xsl_buf + c, "  ");
                    c += sprintf(xsl_buf + c, "<xsl:value-of select=\"'&#10;'\"/>\n");
                }
                else if(!strcmp(argv[i], "-m") || !strcmp(argv[i], "--match"))
                {
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
                
                i++;
            }

            if((i < argc) && (!strcmp(argv[i], "-n") || !strcmp(argv[i], "--nl")))
            {
                for (j=0; j <= m; j++) c += sprintf(xsl_buf + c, "  ");
                c += sprintf(xsl_buf + c, "<xsl:value-of select=\"'&#10;'\"/>\n");
            }

            for (k=0; k<m; k++)
            {
                for (j=k; j<m; j++) c += sprintf(xsl_buf + c, "  ");
                c += sprintf(xsl_buf + c, "</xsl:for-each>\n");
            }

            c += sprintf(xsl_buf + c, "</xsl:template>\n");
        }
        i++;
    }
    
    c += sprintf(xsl_buf + c, "</xsl:stylesheet>\n");

    if (printXSLT) fprintf(stdout, "%s", xsl_buf);

    /*
     *  Parse XSLT stylesheet
     */

    {
        xmlDocPtr style = xmlParseMemory(xsl_buf, c);
        xsltStylesheetPtr cur = xsltParseStylesheetDoc(style);
        xmlDocPtr doc = xmlParseFile("-");
        xsltProcess(doc, cur, "-");
//        xsltFreeStylesheet(cur);
    }
        
    return 0;
}  

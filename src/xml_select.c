/*  $Id: xml_select.c,v 1.2 2002/11/14 02:50:08 mgrouch Exp $  */

#include <string.h>
#include <stdio.h>

#define MAX_XSL_BUF  256*1014

char xsl_buf[MAX_XSL_BUF];




int xml_select(int argc, char **argv)
{
    int c, i, j, k, m, t;
  
    fprintf(stderr, "SELECT\n");
    fprintf(stderr, "Sample: ./xml sel -t -p \"'--------'\" -m \"/xml/*\" -p \"position()\" -t -p \"count(/xml/*)\" -p \"'------FINISH-----'\"\n");

    xsl_buf[0] = 0;
    c = 0;
    
    c += sprintf(xsl_buf, "<?xml version=\"1.0\"?>\n");
    c += sprintf(xsl_buf + c, "<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n");

    c += sprintf(xsl_buf + c, "<xsl:template match=\"/\">\n");       
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
                if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "--print"))
                {
                    for (j=0; j <= m; j++) c += sprintf(xsl_buf + c, "  ");
                    c += sprintf(xsl_buf + c, "<xsl:copy-of select=\"%s\"/>\n", argv[i+1]);
                    i++;
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
    fprintf(stdout, "%s", xsl_buf);
    return 0;
}  

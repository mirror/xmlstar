/*  $Id: xml_select.c,v 1.67 2005/01/07 02:02:13 mgrouch Exp $  */

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

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <libxml/tree.h>

#include "xmlstar.h"
#include "trans.h"

/*
 *  TODO:
 *
 *   1. free memory (check for memory leaks)
 *   2. --nonet option (to disable fetching DTD over network)
 */

#define MAX_NS_ARGS    256

typedef struct _selOptions {
    int printXSLT;        /* Display prepared XSLT */
    int printRoot;        /* Print root element in output (if XML) */
    int outText;          /* Output is text */
    int indent;           /* Indent output */
    int noblanks;         /* Remove insignificant spaces from XML tree */
    int no_omit_decl;     /* Print XML declaration line <?xml version="1.0"?> */
    int nonet;            /* refuse to fetch DTDs or entities over network */
    const xmlChar *encoding; /* the "encoding" attribute on the stylesheet's <xsl:output/> */
} selOptions;

typedef selOptions *selOptionsPtr;

typedef enum { TARG_NONE = 0, TARG_SORT_OP, TARG_XPATH, TARG_STRING,
               TARG_NEWLINE, TARG_NO_CMDLINE = TARG_NEWLINE, TARG_INP_NAME
} template_argument_type;
typedef struct {
    const xmlChar *attrname;
    template_argument_type type;
} template_option_argument;

#define TEMPLATE_OPT_MAX_ARGS 2

typedef struct {
    char shortopt;
    const char *longopt;
    const xmlChar *xslname;
    template_option_argument arguments[TEMPLATE_OPT_MAX_ARGS];
    int nest;
} template_option;


/*
 * usage string chunk : 509 char max on ISO C90
 */
static const char select_usage_str_1[] =
"XMLStarlet Toolkit: Select from XML document(s)\n"
"Usage: %s sel <global-options> {<template>} [ <xml-file> ... ]\n"
"where\n"
"  <global-options> - global options for selecting\n"
"  <xml-file> - input XML document file name/uri (stdin is used if missing)\n"
"  <template> - template for querying XML document with following syntax:\n\n";

static const char select_usage_str_2[] =
"<global-options> are:\n"
"  -C or --comp              - display generated XSLT\n"
"  -R or --root              - print root element <xsl-select>\n"
"  -T or --text              - output is text (default is XML)\n"
"  -I or --indent            - indent output\n"
"  -D or --xml-decl          - do not omit xml declaration line\n"
"  -B or --noblanks          - remove insignificant spaces from XML tree\n"
"  -E or --encode <encoding> - output in the given encoding (utf-8, unicode...)\n";

static const char select_usage_str_3[] =
"  -N <name>=<value>         - predefine namespaces (name without \'xmlns:\')\n"
"                              ex: xsql=urn:oracle-xsql\n"
"                              Multiple -N options are allowed.\n"
"  --net                     - allow fetch DTDs or entities over network\n"
"  --help                    - display help\n\n";

static const char select_usage_str_4[] =
"Syntax for templates: -t|--template <options>\n"
"where <options>\n"
"  -c or --copy-of <xpath>   - print copy of XPATH expression\n"
"  -v or --value-of <xpath>  - print value of XPATH expression\n"
"  -o or --output <string>   - output string literal\n"
"  -n or --nl                - print new line\n"
"  -f or --inp-name          - print input file name (or URL)\n";

static const char select_usage_str_5[] =
"  -m or --match <xpath>     - match XPATH expression\n"
"  -i or --if <test-xpath>   - check condition <xsl:if test=\"test-xpath\">\n"
"  -e or --elem <name>       - print out element <xsl:element name=\"name\">\n"
"  -a or --attr <name>       - add attribute <xsl:attribute name=\"name\">\n"
"  -b or --break             - break nesting\n"
"  -s or --sort op xpath     - sort in order (used after -m) where\n";

static const char select_usage_str_6[] =
"  op is X:Y:Z, \n"
"      X is A - for order=\"ascending\"\n"
"      X is D - for order=\"descending\"\n"
"      Y is N - for data-type=\"numeric\"\n"
"      Y is T - for data-type=\"text\"\n"
"      Z is U - for case-order=\"upper-first\"\n"
"      Z is L - for case-order=\"lower-first\"\n\n";

static const template_option
    OPT_TEMPLATE = { 't', "template" },
    OPT_COPY_OF  = { 'c', "copy-of", BAD_CAST "copy-of", {{BAD_CAST "select", TARG_XPATH}}, 0 },
    OPT_VALUE_OF = { 'v', "value-of", BAD_CAST "value-of", {{BAD_CAST "select", TARG_XPATH}}, 0 },
    OPT_OUTPUT   = { 'o', "output", BAD_CAST "text", {{NULL, TARG_STRING}}, 0 },
    OPT_NL       = { 'n', "nl", BAD_CAST "value-of", {{NULL, TARG_NEWLINE}}, 0 },
    OPT_INP_NAME = { 'f', "inp-name", BAD_CAST "copy-of", {{NULL, TARG_INP_NAME}}, 0 },
    OPT_MATCH    = { 'm', "match", BAD_CAST "for-each", {{BAD_CAST "select", TARG_XPATH}}, 1 },
    OPT_IF       = { 'i', "if", BAD_CAST"if", {{BAD_CAST "test", TARG_XPATH}}, 1 },
    OPT_ELEM     = { 'e', "elem", BAD_CAST "element", {{BAD_CAST "name", TARG_XPATH}}, 1 },
    OPT_ATTR     = { 'a', "attr", BAD_CAST "attribute", {{BAD_CAST "name", TARG_XPATH}}, 1 },
    OPT_BREAK    = { 'b', "break", NULL, {{NULL}}, -1 },
    OPT_SORT     = { 's', "sort", BAD_CAST "sort", {{NULL, TARG_SORT_OP}, {BAD_CAST "select", TARG_XPATH}}, 0 },

    *TEMPLATE_OPTIONS[] = {
        &OPT_TEMPLATE,
        &OPT_COPY_OF,
        &OPT_VALUE_OF,
        &OPT_OUTPUT,
        &OPT_NL,
        &OPT_INP_NAME,
        &OPT_MATCH,
        &OPT_IF,
        &OPT_ELEM,
        &OPT_ATTR,
        &OPT_BREAK,
        &OPT_SORT
    };


static const char select_usage_str_7[] =
"There can be multiple --match, --copy-of, --value-of, etc options\n"
"in a single template. The effect of applying command line templates\n"
"can be illustrated with the following XSLT analogue\n\n"

"xml sel -t -c \"xpath0\" -m \"xpath1\" -m \"xpath2\" -v \"xpath3\" \\\n"
"        -t -m \"xpath4\" -c \"xpath5\"\n\n"

"is equivalent to applying the following XSLT\n\n";

static const char select_usage_str_8[] =
"<?xml version=\"1.0\"?>\n"
"<xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\">\n"
"<xsl:template match=\"/\">\n"
"  <xsl:call-template name=\"t1\"/>\n"
"  <xsl:call-template name=\"t2\"/>\n"
"</xsl:template>\n"
"<xsl:template name=\"t1\">\n"
"  <xsl:copy-of select=\"xpath0\"/>\n"
"  <xsl:for-each select=\"xpath1\">\n"
"    <xsl:for-each select=\"xpath2\">\n"
"      <xsl:value-of select=\"xpath3\"/>\n"
"    </xsl:for-each>\n"
"  </xsl:for-each>\n"
"</xsl:template>\n";

static const char select_usage_str_9[] =
"<xsl:template name=\"t2\">\n"
"  <xsl:for-each select=\"xpath4\">\n"
"    <xsl:copy-of select=\"xpath5\"/>\n"
"  </xsl:for-each>\n"
"</xsl:template>\n"
"</xsl:stylesheet>\n\n";

/**
 *  Print small help for command line options
 */
void
selUsage(const char *argv0, exit_status status)
{
    extern const char more_info[];
    extern const char libxslt_more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprintf(o, select_usage_str_1, argv0);
    fprintf(o, "%s", select_usage_str_2);
    fprintf(o, "%s", select_usage_str_3);
    fprintf(o, "%s", select_usage_str_4);
    fprintf(o, "%s", select_usage_str_5);
    fprintf(o, "%s", select_usage_str_6);
    fprintf(o, "%s", select_usage_str_7);
    fprintf(o, "%s", select_usage_str_8);
    fprintf(o, "%s", select_usage_str_9);
    fprintf(o, "%s", more_info);
    fprintf(o, "%s", libxslt_more_info);
    exit(status);
}

/**
 *  Initialize global command line options
 */
void
selInitOptions(selOptionsPtr ops)
{
    ops->printXSLT = 0;
    ops->printRoot = 0;
    ops->outText = 0;
    ops->indent = 0;
    ops->noblanks = 0;
    ops->no_omit_decl = 0;
    ops->nonet = 1;
    ops->encoding = NULL;
}

/**
 *  Parse command line for additional namespaces
 */
int
selParseNSArr(xmlChar** ns_arr, int* plen,
              int count, char **argv)
{
    int i = 0;
    *plen = 0;
    ns_arr[0] = 0;

    for (i=0; i<count; i++)
    {
        if (argv[i] == 0) break;
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "-N"))
            {
                int j;
                xmlChar *name, *value;

                i++;
                if (i >= count) selUsage(argv[0], EXIT_BAD_ARGS);

                for(j=0; argv[i][j] && (argv[i][j] != '='); j++);
                if (argv[i][j] != '=') selUsage(argv[0], EXIT_BAD_ARGS);

                name = xmlStrndup((const xmlChar *) argv[i], j);
                value = xmlStrdup((const xmlChar *) argv[i]+j+1);

                if (*plen >= MAX_NS_ARGS)
                {
                    fprintf(stderr, "too many namespaces increase MAX_NS_ARGS\n");
                    exit(2);
                }

                ns_arr[*plen] = name;
                (*plen)++;
                ns_arr[*plen] = value;
                (*plen)++;
                ns_arr[*plen] = 0;

                /*printf("xmlns:%s=\"%s\"\n", name, value);*/
            }
        }
        else
            break;
    }

    return i;
}

/**
 *  Cleanup memory allocated by namespaces arguments
 */
void
selCleanupNSArr(xmlChar **ns_arr)
{
    xmlChar **p = ns_arr;

    while (*p)
    {
        xmlFree(*p);
        p++;
    }
}

/**
 *  Parse global command line options
 */
int
selParseOptions(selOptionsPtr ops, int argc, char **argv)
{
    int i;

    i = 2;
    while((i < argc) && (strcmp(argv[i], "-t")) && strcmp(argv[i], "--template"))
    {
        if (!strcmp(argv[i], "-C"))
        {
            ops->printXSLT = 1;
        }
        else if (!strcmp(argv[i], "-B") || !strcmp(argv[i], "--noblanks"))
        {
            ops->noblanks = 1;
        }
        else if (!strcmp(argv[i], "-T") || !strcmp(argv[i], "--text"))
        {
            ops->outText = 1;
        }
        else if (!strcmp(argv[i], "-R") || !strcmp(argv[i], "--root"))
        {
            ops->printRoot = 1;
        }
        else if (!strcmp(argv[i], "-I") || !strcmp(argv[i], "--indent"))
        {
            ops->indent = 1;
        }
        else if (!strcmp(argv[i], "-D") || !strcmp(argv[i], "--xml-decl"))
        {
            ops->no_omit_decl = 1;
        }
        else if (!strcmp(argv[i], "-E") || !strcmp(argv[i], "--encode"))
        {
            if ((i+1) < argc)
            {
                if (argv[i + 1][0] == '-')
                {
                    fprintf(stderr, "-E option requires argument <encoding> ex: (utf-8, unicode...)\n");
                    exit(2);
                }
                else
                {
                    ops->encoding = BAD_CAST argv[i + 1];
                }
            }
            else
            {
                fprintf(stderr, "-E option requires argument <encoding> ex: (utf-8, unicode...)\n");
                exit(2);
            }

        }
        else if (!strcmp(argv[i], "--net"))
        {
            ops->nonet = 0;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") ||
                 !strcmp(argv[i], "-?") || !strcmp(argv[i], "-Z"))
        {
            selUsage(argv[0], EXIT_SUCCESS);
        }
        i++;
    }

    return i;
}

/**
 *  Prepare XSLT template based on command line options
 *  Assumes start points to -t option
 */
int
selGenTemplate(xmlNodePtr root, xmlNsPtr xslns, selOptionsPtr ops,
    const xmlChar *name, int* lastTempl, int start, int argc, char **argv)
{
    int i;
    int templateEmpty;
    int nextTempl;
    const template_option *targ = NULL, *prev_targ = NULL;

    xmlNodePtr template_node = xmlNewChild(root, xslns, BAD_CAST "template", NULL);
    xmlNodePtr node = template_node;

    xmlNewProp(template_node, BAD_CAST "name", name);

    if (strcmp(argv[start], "-t") != 0 &&
        strcmp(argv[start], "--template") != 0)
    {
        fprintf(stderr, "not at the beginning of template\n");
        abort();
    }

    *lastTempl = 0;
    templateEmpty = 1;
    nextTempl = 0;
    i = start + 1;

    while(i < argc)
    {
        xmlNodePtr newnode = NULL;
        int j;

        prev_targ = targ;

        if (argv[i][0] == '-')
        {
            for (j = 0; j < sizeof(TEMPLATE_OPTIONS)/sizeof(*TEMPLATE_OPTIONS); j++)
            {
                targ = TEMPLATE_OPTIONS[j];
                if (argv[i][1] == '-' && strcmp(targ->longopt, &argv[i][2]) == 0)
                    goto found_option; /* long option */
                else if(targ->shortopt == argv[i][1])
                    goto found_option; /* short option */
            }
            fprintf(stderr, "unrecognized option: %s\n", argv[i]);
            abort();
        }
        else
        {
            break;
        }

    found_option:
        if (targ == &OPT_SORT && (prev_targ != &OPT_MATCH && prev_targ != &OPT_SORT))
        {
            fprintf(stderr, "sort(s) must follow match\n");
            exit(2);
        }
        else if (targ == &OPT_TEMPLATE)
        {
            nextTempl = 1;
            i--;
            break;
        }

        i++;
        templateEmpty = 0;

        if (targ->xslname)
            newnode = xmlNewChild(node, xslns, targ->xslname, NULL);

        for (j = 0; j < TEMPLATE_OPT_MAX_ARGS && targ->arguments[j].type; j++)
        {
            if (i >= argc && targ->arguments[j].type < TARG_NO_CMDLINE)
                selUsage(argv[0], EXIT_BAD_ARGS);
            switch (targ->arguments[j].type)
            {
            case TARG_XPATH:
                xmlNewProp(newnode, targ->arguments[j].attrname, BAD_CAST argv[i]);
                break;

            case TARG_STRING:
                xmlNodeAddContent(newnode, BAD_CAST argv[i]);
                break;

            case TARG_NEWLINE:
                xmlNewProp(newnode, BAD_CAST "select", BAD_CAST "'\n'");
                break;

            case TARG_INP_NAME:
                xmlNewProp(newnode, BAD_CAST "select", BAD_CAST "$inputFile");
                break;

            case TARG_SORT_OP: {
                char order, data_type, case_order;
                int nread;
                nread = sscanf(argv[i], "%c:%c:%c", &order, &data_type, &case_order);
                if (nread != 3) selUsage(argv[0], EXIT_BAD_ARGS); /* TODO: allow missing letters */

                if (order == 'A' || order == 'D')
                    xmlNewProp(newnode, BAD_CAST "order",
                        BAD_CAST (order == 'A'? "ascending" : "descending"));
                if (data_type == 'N' || data_type == 'T')
                    xmlNewProp(newnode, BAD_CAST "data_type",
                        BAD_CAST (data_type == 'N'? "numeric" : "text"));
                if (case_order == 'U' || case_order == 'L')
                    xmlNewProp(newnode, BAD_CAST "case_order",
                        BAD_CAST (case_order == 'U'? "upper-first" : "lower-first"));
            } break;

            default:
                assert(0);
            }
            if (targ->arguments[j].type < TARG_NO_CMDLINE) i++;
        }

        switch (targ->nest) {
        case -1:
            node = node->parent;
            break;
        case 0:
            break;
        case 1:
            node = newnode;
            break;
        default:
            assert(0);
        }
    }

    if (templateEmpty)
    {
        fprintf(stderr, "error in arguments:");
        fprintf(stderr, " -t or --template option must be followed by");
        fprintf(stderr, " --match or other options\n");
        exit(3);
    }

    if (!nextTempl)
    {
        if (i >= argc || argv[i][0] != '-' || strcmp(argv[i], "-") == 0)
        {
            *lastTempl = 1;
            return i;           /* return index of next input filename */
        }
    }

    /* return index to beginning of the next template */
    return ++i;
}

/**
 *  Prepare XSLT stylesheet based on command line options
 */
int
selPrepareXslt(xmlDocPtr style, selOptionsPtr ops, xmlChar *ns_arr[],
               int start, int argc, char **argv)
{
    int i, t, ns;
    xmlNodePtr root, root_template;
    xmlNsPtr xslns;
    xmlChar num_buf[1+10+1];    /* d+maxnumber+NUL */

    xslns = xmlNewNs(NULL, BAD_CAST "http://www.w3.org/1999/XSL/Transform",
        BAD_CAST "xsl");
    root = xmlNewDocRawNode(style, xslns, BAD_CAST "stylesheet", NULL);
    xmlDocSetRootElement(style, root);
    xmlNewProp(root, BAD_CAST "version", BAD_CAST "1.0");
    xmlNewNs(root, BAD_CAST "http://www.w3.org/1999/XSL/Transform", BAD_CAST "xsl");
    xmlNewNs(root, BAD_CAST "http://exslt.org/common", BAD_CAST "exslt");
    xmlNewNs(root, BAD_CAST "http://exslt.org/math", BAD_CAST "math");
    xmlNewNs(root, BAD_CAST "http://exslt.org/dates-and-times", BAD_CAST "date");
    xmlNewNs(root, BAD_CAST "http://exslt.org/functions", BAD_CAST "func");
    xmlNewNs(root, BAD_CAST "http://exslt.org/sets", BAD_CAST "set");
    xmlNewNs(root, BAD_CAST "http://exslt.org/strings", BAD_CAST "str");
    xmlNewNs(root, BAD_CAST "http://exslt.org/dynamic", BAD_CAST "dyn");
    xmlNewNs(root, BAD_CAST "http://icl.com/saxon", BAD_CAST "saxon");
    xmlNewNs(root, BAD_CAST "org.apache.xalan.xslt.extensions.Redirect", BAD_CAST "xalanredirect");
    xmlNewNs(root, BAD_CAST "http://www.jclark.com/xt", BAD_CAST "xt");
    xmlNewNs(root, BAD_CAST "http://xmlsoft.org/XSLT/namespace", BAD_CAST "libxslt");
    xmlNewNs(root, BAD_CAST "http://xmlsoft.org/XSLT/", BAD_CAST "test");

    ns = 0;
    while(ns_arr[ns])
    {
        xmlNewNs(root, ns_arr[ns+1], xmlStrlen(ns_arr[ns])?ns_arr[ns] : NULL);
        ns += 2;
    }
    selCleanupNSArr(ns_arr);

    xmlNewProp(root, BAD_CAST "extension-element-prefixes",
        BAD_CAST "exslt math date func set str dyn saxon xalanredirect xt libxslt test");
    xmlNewProp(root, BAD_CAST "exclude-result-prefixes", BAD_CAST "math str");

    {
        xmlNodePtr output;
        output = xmlNewChild(root, xslns, BAD_CAST "output", NULL);
        xmlNewProp(output, BAD_CAST "omit-xml-declaration",
            BAD_CAST ((ops->no_omit_decl)?"no":"yes"));
        xmlNewProp(output, BAD_CAST "indent",
            BAD_CAST ((ops->indent)?"yes":"no"));
        if (ops->encoding) xmlNewProp(output, BAD_CAST "encoding", ops->encoding);
        if (ops->outText) xmlNewProp(output, BAD_CAST "method", BAD_CAST "text");
    }

    {
        xmlNodePtr param;
        param = xmlNewChild(root, xslns, BAD_CAST "param", BAD_CAST "-");
        xmlNewProp(param, BAD_CAST "name", BAD_CAST "inputFile");
    }

    {
        root_template = xmlNewChild(root, xslns, BAD_CAST "template", NULL);
        xmlNewProp(root_template, BAD_CAST "match", BAD_CAST "/");
    }

    if (!ops->outText && ops->printRoot)
        root = xmlNewChild(root, xslns, BAD_CAST "xsl-select", NULL);

    t = 0;
    i = start;
    while(i < argc)
    {
        if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template"))
        {
            xmlNodePtr call_template;
            t++;
            xmlStrPrintf(num_buf, sizeof num_buf, BAD_CAST "t%d", t);
            call_template = xmlNewChild(root_template, xslns, BAD_CAST "call-template", NULL);
            xmlNewProp(call_template, BAD_CAST "name", num_buf);
        }
        i++;
    }

    /*
     *  At least one -t option must be found
     */
    if (t == 0)
    {
        fprintf(stderr, "error in arguments:");
        fprintf(stderr, " no -t or --template options found\n");
        exit(2);
    }

    t = 0;
    i = start;
    while(i < argc)
    {
        if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template"))
        {
            int lastTempl = 0;
            t++;
            xmlStrPrintf(num_buf, sizeof num_buf, BAD_CAST "t%d", t);
            i = selGenTemplate(root, xslns, ops, num_buf, &lastTempl, i, argc, argv);
            if (lastTempl) break;
        }
    }
    return i;
}

/**
 *  This is the main function for 'select' option
 */
int
selMain(int argc, char **argv)
{
    static xsltOptions xsltOps;
    static selOptions ops;
    static const char *params[2 * MAX_PARAMETERS + 1];
    static xmlChar *ns_arr[2 * MAX_NS_ARGS + 1];
    int start, i, n, status = 0;
    int nCount = 0;
    int nbparams;
    xmlDocPtr style_tree;
    xsltStylesheetPtr style;

    if (argc <= 2) selUsage(argv[0], EXIT_BAD_ARGS);

    selInitOptions(&ops);
    xsltInitOptions(&xsltOps);
    start = selParseOptions(&ops, argc, argv);
    xsltOps.nonet = ops.nonet;
    xsltOps.noblanks = ops.noblanks;
    xsltInitLibXml(&xsltOps);

    /* set parameters */
    selParseNSArr(ns_arr, &nCount, start, argv+2);

    style_tree = xmlNewDoc(NULL);
    i = selPrepareXslt(style_tree, &ops, ns_arr, start, argc, argv);

    if (ops.printXSLT)
    {
        xmlDocFormatDump(stdout, style_tree, 1);
        exit(0);
    }

    /*
     *  Parse XSLT stylesheet
     */
    style = xsltParseStylesheetDoc(style_tree);
    if (!style) exit(2);

    for (n=i; n<argc; n++)
    {
        xmlChar *value;

        /*
         *  Pass input file name as predefined parameter 'inputFile'
         */
        nbparams = 2;
        params[0] = "inputFile";
        value = xmlStrdup((const xmlChar *)"'");
        value = xmlStrcat((xmlChar *)value, (const xmlChar *)argv[n]);
        value = xmlStrcat((xmlChar *)value, (const xmlChar *)"'");
        params[1] = (char *) value;

        {
            xmlDocPtr doc = xmlParseFile(argv[n]);
            if (doc != NULL) {
                xsltProcess(&xsltOps, doc, params, style, argv[n]);
            } else {
                status = 2;
            }
        }
        xmlFree(value);
    }

    if (i == argc)
    {
        xmlDocPtr doc;
        nbparams = 2;
        params[0] = "inputFile";
        params[1] = "'-'";

        doc = xmlParseFile("-");
        if (doc != NULL) {
            xsltProcess(&xsltOps, doc, params, style, "-");
        } else {
            status = 2;
        }
    }

    /* 
     * Shutdown libxml
     */
    xsltCleanupGlobals();
    xmlCleanupParser();
    
    return status;
}


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
#include <ctype.h>

#include <libxml/tree.h>
#include <libxslt/templates.h>

#include "xmlstar.h"
#include "trans.h"

#define MAX_NS_ARGS    256
/* max length of xmlstarlet supplied (ie not from command line) namespaces
 * currently xalanredirect is longest, at 13 characters*/
#define MAX_NS_PREFIX_LEN 20

typedef struct {
    const xmlChar *href, *prefix;
} NsEntry;

static const NsEntry ns_entries[] = {
    { BAD_CAST "http://exslt.org/common", BAD_CAST "exslt" },
    { BAD_CAST "http://exslt.org/math", BAD_CAST "math" },
    { BAD_CAST "http://exslt.org/dates-and-times", BAD_CAST "date" },
    { BAD_CAST "http://exslt.org/functions", BAD_CAST "func" },
    { BAD_CAST "http://exslt.org/sets", BAD_CAST "set" },
    { BAD_CAST "http://exslt.org/strings", BAD_CAST "str" },
    { BAD_CAST "http://exslt.org/dynamic", BAD_CAST "dyn" },
    { BAD_CAST "http://icl.com/saxon", BAD_CAST "saxon" },
    { BAD_CAST "org.apache.xalan.xslt.extensions.Redirect",
      BAD_CAST "xalanredirect"}, /* see MAX_NS_PREFIX_LEN */
    { BAD_CAST "http://www.jclark.com/xt", BAD_CAST "xt" },
    { BAD_CAST "http://xmlsoft.org/XSLT/namespace", BAD_CAST "libxslt" },
    { BAD_CAST "http://xmlsoft.org/XSLT/", BAD_CAST "test" },
};

static const NsEntry*
lookup_ns_entry(const char *prefix, int len) {
    int i;
    for (i = 0; i < COUNT_OF(ns_entries); i++) {
        if (xmlStrncmp(BAD_CAST prefix, ns_entries[i].prefix, len) == 0)
            return &ns_entries[i];
    }
    return NULL;
}


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

typedef enum { TARG_NONE = 0, TARG_SORT_OP, TARG_XPATH, TARG_STRING,
               TARG_NEWLINE, TARG_NO_CMDLINE = TARG_NEWLINE, TARG_INP_NAME
} template_argument_type;
typedef struct {
    const xmlChar *attrname;
    template_argument_type type;
} template_option_argument;

#define TEMPLATE_OPT_MAX_ARGS 2
#define TEMPLATE_SUBOPT_MAX 2

typedef struct struct_template_option template_option;

typedef struct {
    const template_option **alternatives;
    int length;
} template_option_alternatives;

typedef enum { ALT_NONE = 0, ALT_TEMPLATES, ALT_SORTS, ALT_BREAK, ALT_ELIF_BREAK
} alt_template_index;

struct struct_template_option {
    char shortopt;
    const char *longopt;
    const xmlChar *xslname;
    template_option_argument arguments[TEMPLATE_OPT_MAX_ARGS];
    const alt_template_index suboptions[TEMPLATE_SUBOPT_MAX];
    const alt_template_index end;
};

const template_option
    OPT_TEMPLATE = { 't', "template", BAD_CAST "template",
                     {{NULL}},
                     {ALT_TEMPLATES}, ALT_BREAK },
    OPT_COPY_OF  = { 'c', "copy-of", BAD_CAST "copy-of",
                     {{BAD_CAST "select", TARG_XPATH}}},
    OPT_VALUE_OF = { 'v', "value-of", BAD_CAST "value-of",
                     {{BAD_CAST "select", TARG_XPATH}}},
    OPT_OUTPUT   = { 'o', "output", BAD_CAST "text", {{NULL, TARG_STRING}}},
    OPT_NL       = { 'n', "nl", BAD_CAST "value-of", {{NULL, TARG_NEWLINE}}},
    OPT_INP_NAME = { 'f', "inp-name", BAD_CAST "copy-of",
                     {{NULL, TARG_INP_NAME}}},
    OPT_MATCH    = { 'm', "match", BAD_CAST "for-each",
                     {{BAD_CAST "select", TARG_XPATH}},
                     {ALT_SORTS, ALT_TEMPLATES}, ALT_BREAK },
    OPT_IF       = { 'i', "if", BAD_CAST "when",
                     {{BAD_CAST "test", TARG_XPATH}},
                     {ALT_TEMPLATES}, ALT_ELIF_BREAK},
    OPT_ELIF     = { 0, "elif", BAD_CAST "when",
                     {{BAD_CAST "test", TARG_XPATH}},
                     {ALT_TEMPLATES}, ALT_ELIF_BREAK},
    OPT_ELSE     = { 0, "else", BAD_CAST "otherwise",
                     {{NULL}},
                     {ALT_TEMPLATES}, ALT_BREAK},
    OPT_ELEM     = { 'e', "elem", BAD_CAST "element",
                     {{BAD_CAST "name", TARG_XPATH}},
                     {ALT_TEMPLATES}, ALT_BREAK },
    OPT_ATTR     = { 'a', "attr", BAD_CAST "attribute",
                     {{BAD_CAST "name", TARG_XPATH}},
                     {ALT_TEMPLATES}, ALT_BREAK },
    OPT_BREAK    = { 'b', "break", NULL, {{NULL}}},
    OPT_SORT     = { 's', "sort", BAD_CAST "sort",
                     {{NULL, TARG_SORT_OP}, {BAD_CAST "select", TARG_XPATH}}},

    *TEMPLATE_ALTERNATIVES[] = {
        &OPT_COPY_OF, &OPT_VALUE_OF, &OPT_OUTPUT, &OPT_NL, &OPT_INP_NAME,
        &OPT_MATCH, &OPT_IF, &OPT_ELEM, &OPT_ATTR
    },
    *TEMPLATE_SORT_ALTERNATIVES[] = {
        &OPT_COPY_OF, &OPT_VALUE_OF, &OPT_OUTPUT, &OPT_NL, &OPT_INP_NAME,
        &OPT_MATCH, &OPT_IF, &OPT_ELEM, &OPT_ATTR, &OPT_SORT
    },
    *TEMPLATE_BREAK_ALTERNATIVES[] = {
        &OPT_BREAK, &OPT_TEMPLATE
    },
    *TEMPLATE_ELIF_BREAK_ALTERNATIVES[] = {
        &OPT_ELIF, &OPT_ELSE, &OPT_BREAK, &OPT_TEMPLATE
    };

#define DEFINE_ALT(alt_array) { alt_array, COUNT_OF(alt_array) }
const template_option_alternatives ALTERNATIVES[] = {
    { NULL, 0 },                /* 1 based array */
    DEFINE_ALT(TEMPLATE_ALTERNATIVES),
    DEFINE_ALT(TEMPLATE_SORT_ALTERNATIVES),
    DEFINE_ALT(TEMPLATE_BREAK_ALTERNATIVES),
    DEFINE_ALT(TEMPLATE_ELIF_BREAK_ALTERNATIVES)
};

void
caseSortFunction(xsltTransformContextPtr ctxt, xmlNodePtr *sorts,
    int nbsorts);

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
                    exit(EXIT_BAD_ARGS);
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
                    exit(EXIT_BAD_ARGS);
                }
                else
                {
                    ops->encoding = BAD_CAST argv[i + 1];
                }
            }
            else
            {
                fprintf(stderr, "-E option requires argument <encoding> ex: (utf-8, unicode...)\n");
                exit(EXIT_BAD_ARGS);
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
 * return option descriptor that matches @option, or NULL if none
 */
static const template_option*
find_alternative(alt_template_index alt, const char *option)
{
    int i;
    if (alt) for (i = 0; i < ALTERNATIVES[alt].length; i++)
    {
        const template_option *targ = ALTERNATIVES[alt].alternatives[i];
        if ((option[1] == '-' && strcmp(targ->longopt, &option[2]) == 0) ||
            targ->shortopt == option[1])
            return targ;
    }
    return NULL;
}
/**
 *  Prepare XSLT template based on command line options
 *  Assumes start points to -t option
 */
int
selGenTemplate(xmlNodePtr root, xmlNodePtr template_node,
    xmlNsPtr xslns, selOptionsPtr ops, int* lastTempl,
    int start, int argc, char **argv)
{
    int i;
    int templateEmpty;
    int nextTempl;
    const template_option *targ = NULL;
    xmlNodePtr node = template_node;

    if (strcmp(argv[start], "-t") != 0 &&
        strcmp(argv[start], "--template") != 0)
    {
        fprintf(stderr, "not at the beginning of template\n");
        abort();
    }
    targ = &OPT_TEMPLATE;

    *lastTempl = 0;
    templateEmpty = 1;
    nextTempl = 0;
    i = start + 1;

    while(i < argc)
    {
        int j;
        xmlNodePtr newnode = NULL;
        const template_option *newtarg = NULL;

        if (argv[i][0] == '-')
        {
            newtarg = find_alternative(targ->suboptions[0], argv[i]);
            if (newtarg) goto found_option;
            newtarg = find_alternative(targ->suboptions[1], argv[i]);
            if (newtarg) goto found_option;
            newtarg = find_alternative(targ->end, argv[i]);
            if (newtarg) {
                node = node->parent;
                if ((targ == &OPT_IF || targ == &OPT_ELIF || targ == &OPT_ELSE)
                    && newtarg == &OPT_BREAK)
                    node = node->parent;
                targ = node->_private;
                goto found_option;
            }

            fprintf(stderr, "unrecognized option: %s\n", argv[i]);
            exit(EXIT_BAD_ARGS);
        }
        else
        {
            break;
        }

    found_option:
        if (newtarg == &OPT_TEMPLATE)
        {
            nextTempl = 1;
            i--;
            break;
        }
        else if (newtarg == &OPT_IF)
        {
            node->_private = (void*) targ;
            targ = newtarg;
            node = xmlNewChild(node, xslns, BAD_CAST "choose", NULL);
        }

        i++;
        templateEmpty = 0;

        if (newtarg->xslname)
            newnode = xmlNewChild(node, xslns, newtarg->xslname, NULL);

        for (j = 0; j < TEMPLATE_OPT_MAX_ARGS && newtarg->arguments[j].type; j++)
        {
            if (i >= argc && newtarg->arguments[j].type < TARG_NO_CMDLINE)
                selUsage(argv[0], EXIT_BAD_ARGS);
            switch (newtarg->arguments[j].type)
            {
            case TARG_XPATH: {
                /* Search for namespace references. Note that we might pickup
                 * things that aren't actually namespace references because we
                 * don't have a full XPath parser. That's okay, an extra
                 * namespace definition won't hurt anyone. */
                const char *colon;
                for (colon = argv[i]; colon; colon++) {
                    int ns_idx = -1;

                    colon = strchr(colon, ':');
                    if (!colon) break;

                    for (;; ns_idx--) {
                        if (&colon[ns_idx] < argv[i]
                            ||!isalnum(colon[ns_idx])) {
                            const NsEntry *ns;
                            ns_idx++;
                            ns = lookup_ns_entry(&colon[ns_idx], -ns_idx);
                            if (ns) xmlNewNs(root, ns->href, ns->prefix);
                            break;
                        }
                        if (-ns_idx >= MAX_NS_PREFIX_LEN) break;
                    }
                }
                xmlNewProp(newnode, newtarg->arguments[j].attrname, BAD_CAST argv[i]);
                break;
            }

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
                    xmlNewProp(newnode, BAD_CAST "data-type",
                        BAD_CAST (data_type == 'N'? "number" : "text"));
                if (case_order == 'U' || case_order == 'L')
                    xmlNewProp(newnode, BAD_CAST "case-order",
                        BAD_CAST (case_order == 'U'? "upper-first" : "lower-first"));
            } break;

            default:
                assert(0);
            }
            if (newtarg->arguments[j].type < TARG_NO_CMDLINE) i++;
        }

        if (*newtarg->suboptions) {
            node->_private = (void*) targ;
            node = newnode;
            targ = newtarg;
        }
    }

    if (templateEmpty)
    {
        fprintf(stderr, "error in arguments:");
        fprintf(stderr, " -t or --template option must be followed by");
        fprintf(stderr, " --match or other options\n");
        exit(EXIT_BAD_ARGS);
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
    xmlNodePtr root, root_template = NULL;
    xmlNsPtr xslns;
    xmlBufferPtr attr_buf;

    root = xmlNewDocRawNode(style, NULL, BAD_CAST "stylesheet", NULL);
    xmlDocSetRootElement(style, root);
    xmlNewProp(root, BAD_CAST "version", BAD_CAST "1.0");
    xslns = xmlNewNs(root, BAD_CAST "http://www.w3.org/1999/XSL/Transform", BAD_CAST "xsl");
    xmlSetNs(root, xslns);

    ns = 0;
    while(ns_arr[ns])
    {
        xmlNewNs(root, ns_arr[ns+1], xmlStrlen(ns_arr[ns])?ns_arr[ns] : NULL);
        ns += 2;
    }
    selCleanupNSArr(ns_arr);

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

    for (i = start, t = 0; i < argc; i++)
        if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template"))
            t++;

    /*
     *  At least one -t option must be found
     */
    if (t == 0)
    {
        fprintf(stderr, "error in arguments:");
        fprintf(stderr, " no -t or --template options found\n");
        exit(EXIT_BAD_ARGS);
    }

    if (t > 1)
        root_template = xmlNewChild(root, xslns, BAD_CAST "template", NULL);

    t = 0;
    i = start;
    while(i < argc)
    {
        if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--template"))
        {
            xmlNodePtr call_template, template;
            int lastTempl = 0;
            t++;
            template = xmlNewChild(root, xslns, BAD_CAST "template", NULL);

            if (root_template) {
                xmlChar num_buf[1+10+1];    /* t+maxnumber+NUL */
                xmlStrPrintf(num_buf, sizeof num_buf, BAD_CAST "t%d", t);

                call_template = xmlNewChild(root_template, xslns,
                    BAD_CAST "call-template", NULL);
                xmlNewProp(call_template, BAD_CAST "name", num_buf);
                xmlNewProp(template, BAD_CAST "name", num_buf);
            } else {
                root_template = template;
            }

            i = selGenTemplate(root, template,
                xslns, ops, &lastTempl, i, argc, argv);
            if (lastTempl) break;
        }
    }

    if (!ops->outText && ops->printRoot) {
        xmlNodePtr result_root = root_template;
        xmlNodeSetName(result_root, BAD_CAST "xsl-select");
        xmlSetNs(result_root, NULL);
        xmlUnlinkNode(result_root);

        root_template = xmlNewChild(root, xslns, BAD_CAST "template", NULL);
        xmlAddChild(root_template, result_root);
    }

    xmlNewProp(root_template, BAD_CAST "match", BAD_CAST "/");

    attr_buf = xmlBufferCreate();
    for (ns = 0; ns < COUNT_OF(ns_entries); ns++) {
        if (xmlSearchNs(NULL, root, ns_entries[ns].prefix)) {
            if (xmlBufferLength(attr_buf) != 0)
                xmlBufferWriteChar(attr_buf, " ");
            xmlBufferCat(attr_buf, ns_entries[ns].prefix);
        }
    }
    if (xmlBufferLength(attr_buf) != 0)
        xmlNewProp(root, BAD_CAST "extension-element-prefixes",
            xmlBufferContent(attr_buf));

    xmlBufferFree(attr_buf);

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
    xsltSetSortFunc(caseSortFunction);

    /* set parameters */
    selParseNSArr(ns_arr, &nCount, start, argv+2);

    style_tree = xmlNewDoc(NULL);
    i = selPrepareXslt(style_tree, &ops, ns_arr, start, argc, argv);

    if (ops.printXSLT)
    {
        xmlDocFormatDump(stdout, style_tree, 1);
        exit(EXIT_SUCCESS);
    }

    /*
     *  Parse XSLT stylesheet
     */
    style = xsltParseStylesheetDoc(style_tree);
    if (!style) exit(EXIT_LIB_ERROR);

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
    xsltFreeStylesheet(style);
    xsltCleanupGlobals();
    xmlCleanupParser();
    
    return status;
}




/****************************************************************************/

/**
 * @number: compare numerically?
 * @returns: negative if @obj1 compares less than @obj2
 */
static int
compareFunction(xmlXPathObjectPtr obj1, xmlXPathObjectPtr obj2,
    int number, int lower_first, int descending)
{
    int tst;

    if (number) {
        /* We make NaN smaller than number in accordance
           with XSLT spec */
        if (xmlXPathIsNaN(obj1->floatval)) {
            if (xmlXPathIsNaN(obj2->floatval))
                tst = 0;
            else
                tst = -1;
        } else if (xmlXPathIsNaN(obj2->floatval))
            tst = 1;
        else if (obj1->floatval == obj2->floatval)
            tst = 0;
        else if (obj1->floatval > obj2->floatval)
            tst = 1;
        else tst = -1;
    } else {
        tst = xmlStrcasecmp(obj1->stringval, obj2->stringval);
        if (tst == 0) {
            tst = xmlStrcmp(obj1->stringval, obj2->stringval);
            if (lower_first)
                tst = -tst;
        }
    }
    if (descending)
        tst = -tst;

    return tst;
}

/**
 * xsltSortFunction:
 * @ctxt:  a XSLT process context
 * @sorts:  array of sort nodes
 * @nbsorts:  the number of sorts in the array
 *
 * reorder the current node list accordingly to the set of sorting
 * requirement provided by the arry of nodes.
 *
 * like xsltDefaultSortFunction, but respect case-order attribute
 */
void
caseSortFunction(xsltTransformContextPtr ctxt, xmlNodePtr *sorts,
	           int nbsorts) {
#ifdef XSLT_REFACTORED
    xsltStyleItemSortPtr comp;
#else
    xsltStylePreCompPtr comp;
#endif
    xmlXPathObjectPtr *resultsTab[XSLT_MAX_SORT];
    xmlXPathObjectPtr *results = NULL, *res;
    xmlNodeSetPtr list = NULL;
    int descending, number, desc, numb;
    int len = 0;
    int i, j, incr;
    int tst;
    int depth;
    xmlNodePtr node;
    xmlXPathObjectPtr tmp;
    int tempstype[XSLT_MAX_SORT], temporder[XSLT_MAX_SORT],
        tempcaseorder[XSLT_MAX_SORT];

    if ((ctxt == NULL) || (sorts == NULL) || (nbsorts <= 0) ||
	(nbsorts >= XSLT_MAX_SORT))
	return;
    if (sorts[0] == NULL)
	return;
    comp = sorts[0]->psvi;
    if (comp == NULL)
	return;

    list = ctxt->nodeList;
    if ((list == NULL) || (list->nodeNr <= 1))
	return; /* nothing to do */

    for (j = 0; j < nbsorts; j++) {
	comp = sorts[j]->psvi;
	tempstype[j] = 0;
	if ((comp->stype == NULL) && (comp->has_stype != 0)) {
	    comp->stype =
		xsltEvalAttrValueTemplate(ctxt, sorts[j],
					  (const xmlChar *) "data-type",
					  XSLT_NAMESPACE);
	    if (comp->stype != NULL) {
		tempstype[j] = 1;
		if (xmlStrEqual(comp->stype, (const xmlChar *) "text"))
		    comp->number = 0;
		else if (xmlStrEqual(comp->stype, (const xmlChar *) "number"))
		    comp->number = 1;
		else {
		    xsltTransformError(ctxt, NULL, sorts[j],
			  "xsltDoSortFunction: no support for data-type = %s\n",
				     comp->stype);
		    comp->number = 0; /* use default */
		}
	    }
	}
	temporder[j] = 0;
	if ((comp->order == NULL) && (comp->has_order != 0)) {
	    comp->order = xsltEvalAttrValueTemplate(ctxt, sorts[j],
						    (const xmlChar *) "order",
						    XSLT_NAMESPACE);
	    if (comp->order != NULL) {
		temporder[j] = 1;
		if (xmlStrEqual(comp->order, (const xmlChar *) "ascending"))
		    comp->descending = 0;
		else if (xmlStrEqual(comp->order,
				     (const xmlChar *) "descending"))
		    comp->descending = 1;
		else {
		    xsltTransformError(ctxt, NULL, sorts[j],
			     "xsltDoSortFunction: invalid value %s for order\n",
				     comp->order);
		    comp->descending = 0; /* use default */
		}
	    }
	}

        tempcaseorder[j] = 0;
	if ((comp->case_order == NULL) /* && (comp->has_case_order != 0) */) {
	    comp->case_order = xsltEvalAttrValueTemplate(ctxt, sorts[j],
                (const xmlChar *) "case-order", XSLT_NAMESPACE);
	    if (comp->case_order != NULL) {
		tempcaseorder[j] = 1;
		if (xmlStrEqual(comp->case_order, BAD_CAST "upper-first"))
		    comp->lower_first = 0;
		else if (xmlStrEqual(comp->case_order, BAD_CAST "lower-first"))
		    comp->lower_first = 1;
		else {
		    xsltTransformError(ctxt, NULL, sorts[j],
                        "xsltDoSortFunction: invalid value %s for case-order\n",
                        comp->case_order);
		    comp->lower_first = 0; /* use default */
		}
	    }
	}
    }

    len = list->nodeNr;

    resultsTab[0] = xsltComputeSortResult(ctxt, sorts[0]);
    for (i = 1;i < XSLT_MAX_SORT;i++)
	resultsTab[i] = NULL;

    results = resultsTab[0];

    comp = sorts[0]->psvi;
    descending = comp->descending;
    number = comp->number;
    if (results == NULL)
	return;

    /* Shell's sort of node-set */
    for (incr = len / 2; incr > 0; incr /= 2) {
	for (i = incr; i < len; i++) {
	    j = i - incr;
	    if (results[i] == NULL)
		continue;

	    while (j >= 0) {
                if (results[j] == NULL)
                    tst = 1;
                else
                    tst = compareFunction(results[j], results[j + incr],
                        number, comp->lower_first, descending);

		if (tst == 0) {
		    /*
		     * Okay we need to use multi level sorts
		     */
		    depth = 1;
		    while (depth < nbsorts) {
			if (sorts[depth] == NULL)
			    break;
			comp = sorts[depth]->psvi;
			if (comp == NULL)
			    break;
			desc = comp->descending;
			numb = comp->number;

			/*
			 * Compute the result of the next level for the
			 * full set, this might be optimized ... or not
			 */
			if (resultsTab[depth] == NULL)
			    resultsTab[depth] = xsltComputeSortResult(ctxt,
				                        sorts[depth]);
			res = resultsTab[depth];
			if (res == NULL)
			    break;
			if (res[j] == NULL) {
			    if (res[j+incr] != NULL)
				tst = 1;
			} else {
                            tst = compareFunction(res[j], res[j+incr],
                                numb, comp->lower_first, desc);
                        }

			/*
			 * if we still can't differenciate at this level
			 * try one level deeper.
			 */
			if (tst != 0)
			    break;
			depth++;
		    }
		}
		if (tst == 0) {
		    tst = results[j]->index > results[j + incr]->index;
		}
		if (tst > 0) {
		    tmp = results[j];
		    results[j] = results[j + incr];
		    results[j + incr] = tmp;
		    node = list->nodeTab[j];
		    list->nodeTab[j] = list->nodeTab[j + incr];
		    list->nodeTab[j + incr] = node;
		    depth = 1;
		    while (depth < nbsorts) {
			if (sorts[depth] == NULL)
			    break;
			if (resultsTab[depth] == NULL)
			    break;
			res = resultsTab[depth];
			tmp = res[j];
			res[j] = res[j + incr];
			res[j + incr] = tmp;
			depth++;
		    }
		    j -= incr;
		} else
		    break;
	    }
	}
    }

    for (j = 0; j < nbsorts; j++) {
	comp = sorts[j]->psvi;
	if (tempstype[j] == 1) {
	    /* The data-type needs to be recomputed each time */
	    xmlFree((void *)(comp->stype));
	    comp->stype = NULL;
	}
	if (temporder[j] == 1) {
	    /* The order needs to be recomputed each time */
	    xmlFree((void *)(comp->order));
	    comp->order = NULL;
	}
	if (tempcaseorder[j] == 1) {
	    /* The case-order needs to be recomputed each time */
	    xmlFree((void *)(comp->case_order));
	    comp->case_order = NULL;
	}
	if (resultsTab[j] != NULL) {
	    for (i = 0;i < len;i++)
		xmlXPathFreeObject(resultsTab[j][i]);
	    xmlFree(resultsTab[j]);
	}
    }
}

/*  $Id: xml_edit.c,v 1.45 2005/01/08 00:07:03 mgrouch Exp $  */

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
#include <libxml/xmlsave.h>
#include <libxml/HTMLtree.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xpointer.h>
#include <libxml/parserInternals.h>
#include <libxml/uri.h>
#include <libexslt/exslt.h>

#include "xmlstar.h"

/*
   TODO:
          1. Should this be allowed ?
             ./xml ed -m /xml /xml/table/rec/object ../examples/xml/tab-obj.xml 
*/

typedef struct _edOptions {   /* Global 'edit' options */
    int noblanks;             /* Remove insignificant spaces from XML tree */
    int preserveFormat;       /* Preserve original XML formatting */
    int omit_decl;            /* Omit XML declaration line <?xml version="1.0"?> */
    int inplace;              /* Edit file inplace (no output on stdout) */
    int nonet;                /* Disallow network access */
} edOptions;

typedef edOptions *edOptionsPtr;

typedef enum _XmlEdOp {
   XML_ED_DELETE,
   XML_ED_INSERT,
   XML_ED_APPEND,
   XML_ED_UPDATE,
   XML_ED_RENAME,
   XML_ED_MOVE,
   XML_ED_SUBNODE   
} XmlEdOp;

/* TODO ??? */
typedef enum _XmlNodeType {
   XML_UNDEFINED,
   XML_ATTR,
   XML_ELEM,
   XML_TEXT,
   XML_COMT,
   XML_CDATA,
   XML_EXPR
} XmlNodeType;

typedef char* XmlEdArg;

typedef struct _XmlEdAction {
  XmlEdOp       op;
  XmlEdArg      arg1;
  XmlEdArg      arg2;
  XmlEdArg      arg3;
  XmlNodeType   type;
} XmlEdAction;

#define MAX_XML_ED_OPS 128

int ops_count = 0;
static XmlEdAction ops[MAX_XML_ED_OPS];

/*
 * usage string chunk : 509 char max on ISO C90
 */
static const char edit_usage_str_1[] =
"XMLStarlet Toolkit: Edit XML document(s)\n"
"Usage: %s ed <global-options> {<action>} [ <xml-file-or-uri> ... ]\n"
"where\n"
"  <global-options>  - global options for editing\n"
"  <xml-file-or-uri> - input XML document file name/uri (stdin otherwise)\n\n";


static const char edit_usage_str_2[] =
"<global-options> are:\n"
"  -P, or -S           - preserve whitespace nodes.\n"
"     (or --pf, --ps)    Note that space between attributes is not preserved\n"
"  -O (or --omit-decl) - omit XML declaration (<?xml ...?>)\n"
"  -L (or --inplace)   - edit file inplace\n"
"  -N <name>=<value>   - predefine namespaces (name without \'xmlns:\')\n"
"                        ex: xsql=urn:oracle-xsql\n"
"                        Multiple -N options are allowed.\n"
"                        -N options must be last global options.\n";

static const char edit_usage_str_3[] =
"  --net               - allow network access\n"
"  --help or -h        - display help\n\n"
"where <action>\n"
"  -d or --delete <xpath>\n"
"  -i or --insert <xpath> -t (--type) elem|text|attr -n <name> -v (--value) <value>\n"
"  -a or --append <xpath> -t (--type) elem|text|attr -n <name> -v (--value) <value>\n"
"  -s or --subnode <xpath> -t (--type) elem|text|attr -n <name> -v (--value) <value>\n";

static const char edit_usage_str_4[] =
"  -m or --move <xpath1> <xpath2>\n"
"  -r or --rename <xpath1> -v <new-name>\n"
"  -u or --update <xpath> -v (--value) <value>\n"
"                         -x (--expr) <xpath>\n\n";

/**
 *  display short help message
 */
void
edUsage(const char *argv0, exit_status status)
{
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprintf(o, edit_usage_str_1, argv0);
    fprintf(o, "%s", edit_usage_str_2);
    fprintf(o, "%s", edit_usage_str_3);
    fprintf(o, "%s", edit_usage_str_4);
    fprintf(o, "%s", more_info);
    exit(status);
}

/**
 *  Initialize global command line options
 */
void
edInitOptions(edOptionsPtr ops)
{
    ops->noblanks = 1;
    ops->omit_decl = 0;
    ops->preserveFormat = 0;
    ops->inplace = 0;
    ops->nonet = 1;
}

/**
 *  Parse global command line options
 */
int
edParseOptions(edOptionsPtr ops, int argc, char **argv)
{
    int i;

    i = 2;
    while((i < argc) && (argv[i][0] == '-'))
    {
        if (!strcmp(argv[i], "-S") || !strcmp(argv[i], "--ps"))
        {
            ops->noblanks = 0;            /* preserve spaces */
        }
        else if (!strcmp(argv[i], "-P") || !strcmp(argv[i], "--pf"))
        {
            ops->preserveFormat = 1;      /* preserve format */
        }
        else if (!strcmp(argv[i], "-O") || !strcmp(argv[i], "--omit-decl"))
        {
            ops->omit_decl = 1;
        }
        else if (!strcmp(argv[i], "-L") || !strcmp(argv[i], "--inplace"))
        {
            ops->inplace = 1;
        }
        else if (!strcmp(argv[i], "--net"))
        {
            ops->nonet = 0;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") ||
                 !strcmp(argv[i], "-?") || !strcmp(argv[i], "-Z"))
        {
            edUsage(argv[0], EXIT_SUCCESS);
        }
        else
        {
            break;
        }
        i++;
    }

    return i;
}

/**
 *  register the namespace from @ns_arr to @ctxt
 */
void
nsarr_xpath_register(xmlXPathContextPtr ctxt)
{
    int ns;
    for (ns = 0; ns_arr[ns]; ns += 2) {
        xmlXPathRegisterNs(ctxt, ns_arr[ns], ns_arr[ns+1]);
    }
}

/**
 * register top-level namespace definitions from @doc to @ctxt
 */
static void
extract_ns_defs(xmlDocPtr doc, xmlXPathContextPtr ctxt)
{
    xmlNsPtr nsDef;
    xmlNodePtr root = xmlDocGetRootElement(doc);
    if (!root) return;

    for (nsDef = root->nsDef; nsDef; nsDef = nsDef->next) {
        if (nsDef->prefix != NULL) /* can only register ns with prefix */
            xmlXPathRegisterNs(ctxt, nsDef->prefix, nsDef->href);
    }
}

/**
 *  'update' operation
 */
void
edUpdate(xmlDocPtr doc, xmlNodeSetPtr nodes, const char *val,
    XmlNodeType type, xmlXPathContextPtr ctxt)
{
    int i;
    xmlXPathCompExprPtr xpath = NULL;
    xmlNodePtr ctxt_node = ctxt->node;

    if (type == XML_EXPR) {
        xpath = xmlXPathCompile((const xmlChar*) val);
        if (!xpath) return;
    }

    for (i = 0; i < nodes->nodeNr; i++)
    {
        /* update node */
        if (type == XML_EXPR) {
            xmlXPathObjectPtr res;
            xmlChar *string;

            ctxt->node = nodes->nodeTab[i];
            res = xmlXPathConvertString(xmlXPathCompiledEval(xpath, ctxt));
            /* TODO: do we need xmlEncodeEntitiesReentrant() too/instead? */
            string = xmlEncodeSpecialChars(doc, res->stringval);
            xmlXPathFreeObject(res);
            xmlNodeSetContent(nodes->nodeTab[i], string);
            xmlFree(string);
        } else {
            /* TODO: do we need xmlEncodeEntitiesReentrant() too/instead? */
            xmlChar *content = xmlEncodeSpecialChars(NULL, (const xmlChar*) val);
            xmlNodeSetContent(nodes->nodeTab[i], content);
            xmlFree(content);
        }
    }

    xmlXPathFreeCompExpr(xpath);
    ctxt->node = ctxt_node;
}

/**
 *  'insert' operation
 */
void
edInsert(xmlDocPtr doc, xmlNodeSetPtr nodes, const char *val, const char *name,
         XmlNodeType type, int mode)
{
    int i;
    for (i = 0; i < nodes->nodeNr; i++)
    {
        /* update node */
        if (type == XML_ATTR)
        {
            xmlNewProp(nodes->nodeTab[i], BAD_CAST name, BAD_CAST val);
        }
        else if (type == XML_ELEM)
        {
            xmlNodePtr node = xmlNewDocNode(doc, NULL /* TODO: NS */, BAD_CAST name, BAD_CAST val);
            if (mode > 0)
                xmlAddNextSibling(nodes->nodeTab[i], node);
            else if (mode < 0)
                xmlAddPrevSibling(nodes->nodeTab[i], node);
            else
                xmlAddChild(nodes->nodeTab[i], node);
        }
        else if (type == XML_TEXT)
        {
            xmlNodePtr node = xmlNewDocText(doc, BAD_CAST val);
            if (mode > 0)
                xmlAddNextSibling(nodes->nodeTab[i], node);
            else if (mode < 0)
                xmlAddPrevSibling(nodes->nodeTab[i], node);
            else
                xmlAddChild(nodes->nodeTab[i], node);
        }
    }
}

/**
 *  'rename' operation
 */
void
edRename(xmlDocPtr doc, xmlNodeSetPtr nodes, char *val, XmlNodeType type)
{
    int i;
    for (i = 0; i < nodes->nodeNr; i++)
    {
        xmlNodeSetName(nodes->nodeTab[i], BAD_CAST val);
    }
}

/**
 *  'delete' operation
 */
void
edDelete(xmlDocPtr doc, xmlNodeSetPtr nodes)
{
    int i;
    for (i = nodes->nodeNr - 1; i >= 0; i--)
    {
        if (nodes->nodeTab[i]->type == XML_NAMESPACE_DECL) {
            fprintf(stderr, "FIXME: can't delete namespace nodes\n");
            exit(EXIT_INTERNAL_ERROR);
        }
        /* delete node */
        xmlUnlinkNode(nodes->nodeTab[i]);

        /* Free node and children */
        xmlFreeNode(nodes->nodeTab[i]);
        nodes->nodeTab[i] = NULL;
    }
}

/**
 *  'move' operation
 */
void
edMove(xmlDocPtr doc, xmlNodeSetPtr nodes, xmlNodePtr to)
{
    int i;
    for (i = 0; i < nodes->nodeNr; i++)
    {
        if (nodes->nodeTab[i]->type == XML_NAMESPACE_DECL) {
            fprintf(stderr, "FIXME: can't move namespace nodes\n");
            exit(EXIT_INTERNAL_ERROR);
        }
        /* move node */
        xmlUnlinkNode(nodes->nodeTab[i]);
        xmlAddChild(to, nodes->nodeTab[i]);
    }
}

/**
 *  Loop through array of operations and perform them
 */
void
edProcess(xmlDocPtr doc, XmlEdAction* ops, int ops_count)
{
    int k;
    xmlXPathContextPtr ctxt = xmlXPathNewContext(doc);
    /* NOTE: later registrations override earlier ones */
#if HAVE_EXSLT_XPATH_REGISTER
    /* register extension functions */
    exsltDateXpathCtxtRegister(ctxt, BAD_CAST "date");
    exsltMathXpathCtxtRegister(ctxt, BAD_CAST "math");
    exsltSetsXpathCtxtRegister(ctxt, BAD_CAST "set");
    exsltStrXpathCtxtRegister(ctxt, BAD_CAST "str");
#endif
    /* namespaces from doc */
    extract_ns_defs(doc, ctxt);
    /* namespaces from command line */
    nsarr_xpath_register(ctxt);
    ctxt->node = xmlDocGetRootElement(doc);

    for (k = 0; k < ops_count; k++)
    {
        xmlXPathObjectPtr res;
        xmlNodeSetPtr nodes;

        res = xmlXPathEvalExpression(BAD_CAST ops[k].arg1, ctxt);
        if (!res || res->type != XPATH_NODESET || !res->nodesetval) continue;
        nodes = res->nodesetval;

        switch (ops[k].op)
        {
            case XML_ED_DELETE:
                edDelete(doc, nodes);
                break;
            case XML_ED_MOVE: {
                xmlXPathObjectPtr res_to =
                    xmlXPathEvalExpression(BAD_CAST ops[k].arg2, ctxt);
                if (!res_to
                    || res_to->type != XPATH_NODESET
                    || res_to->nodesetval->nodeNr != 1) {
                    fprintf(stderr, "move destination is not a single node\n");
                    continue;
                }
                edMove(doc, nodes, res_to->nodesetval->nodeTab[0]);
                xmlXPathFreeObject(res_to);
                break;
            }
            case XML_ED_UPDATE:
                edUpdate(doc, nodes, ops[k].arg2, ops[k].type, ctxt);
                break;
            case XML_ED_RENAME:
                edRename(doc, nodes, ops[k].arg2, ops[k].type);
                break;
            case XML_ED_INSERT:
                edInsert(doc, nodes, ops[k].arg2, ops[k].arg3, ops[k].type, -1);
                break;
            case XML_ED_APPEND:
                edInsert(doc, nodes, ops[k].arg2, ops[k].arg3, ops[k].type, 1);
                break;
            case XML_ED_SUBNODE:
                edInsert(doc, nodes, ops[k].arg2, ops[k].arg3, ops[k].type, 0);
                break;
            default:
                break;
        }
        xmlXPathFreeObject(res);
    }
    xmlXPathFreeContext(ctxt);
}

/**
 *  Output document
 */
void
edOutput(const char* filename, edOptions g_ops)
{
    xmlDocPtr doc;
    int save_options =
#if LIBXML_VERSION >= 20708
        (g_ops.noblanks? 0 : XML_SAVE_WSNONSIG) |
#endif
        (g_ops.preserveFormat? 0 : XML_SAVE_FORMAT) |
        (g_ops.omit_decl? XML_SAVE_NO_DECL : 0);
    int read_options =
        (g_ops.nonet? XML_PARSE_NONET : 0);
    xmlSaveCtxtPtr save;

    doc = xmlReadFile(filename, NULL, read_options);
    if (!doc)
    {
        cleanupNSArr(ns_arr);
        xmlCleanupParser();
        xmlCleanupGlobals();
        exit(EXIT_BAD_FILE);
    }

    edProcess(doc, ops, ops_count);

    /* avoid getting ASCII CRs in UTF-16/UCS-(2,4) text */
    if ((xmlStrcasestr(doc->encoding, BAD_CAST "UTF") == 0
            && xmlStrcasestr(doc->encoding, BAD_CAST "16") == 0)
        ||
        (xmlStrcasestr(doc->encoding, BAD_CAST "UCS") == 0
            && (xmlStrcasestr(doc->encoding, BAD_CAST "2") == 0
                ||
                xmlStrcasestr(doc->encoding, BAD_CAST "4") == 0)))
    {
        set_stdout_binary();
    }

    save = xmlSaveToFilename(g_ops.inplace? filename : "-", NULL, save_options);
    xmlSaveDoc(save, doc);
    xmlSaveClose(save);

    xmlFreeDoc(doc);
}

/**
 *  This is the main function for 'edit' option
 */
int
edMain(int argc, char **argv)
{
    int i, j, n, start = 0;
    static edOptions g_ops;
    int nCount = 0;

    if (argc < 3) edUsage(argv[0], EXIT_BAD_ARGS);

    edInitOptions(&g_ops);
    start = edParseOptions(&g_ops, argc, argv);

    parseNSArr(ns_arr, &nCount, argc-start, argv+start);
        
    /*
     *  Parse command line and fill array of operations
     */
    j = 0;
    i = start + nCount;

    while (i < argc)
    {
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--delete"))
        {
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);
            ops[j].op = XML_ED_DELETE;
            ops[j].arg1 = argv[i];
            ops[j].arg2 = 0;
            ops[j].type = XML_UNDEFINED;
            j++;            
        }
        else if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--move"))
        {
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);
            ops[j].op = XML_ED_MOVE;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);
            ops[j].arg2 = argv[i];
            ops[j].type = XML_UNDEFINED;
            j++;
        }
        else if (!strcmp(argv[i], "-u") || !strcmp(argv[i], "--update"))
        {
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);
            ops[j].type = XML_UNDEFINED;
            ops[j].op = XML_ED_UPDATE;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);
            if (strcmp(argv[i], "-v") && strcmp(argv[i], "--value") &&
                strcmp(argv[i], "-x") && strcmp(argv[i], "--expr"))
                edUsage(argv[0], EXIT_BAD_ARGS);

            if (!strcmp(argv[i], "-x") || !strcmp(argv[i], "--expr"))
                ops[j].type = XML_EXPR;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);
            ops[j].arg2 = argv[i];

            j++;
        }
        else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--rename"))
        {
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);
            ops[j].type = XML_UNDEFINED;
            ops[j].op = XML_ED_RENAME;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);
            if (strcmp(argv[i], "-v") && strcmp(argv[i], "--value"))
                edUsage(argv[0], EXIT_BAD_ARGS);
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);
            ops[j].arg2 = argv[i];
            j++;
        }
        else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--insert"))
        {
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);
            ops[j].type = XML_UNDEFINED;
            ops[j].op = XML_ED_INSERT;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            if (strcmp(argv[i], "-t") && strcmp(argv[i], "--type")) edUsage(argv[0], EXIT_BAD_ARGS);;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            if (!strcmp(argv[i], "elem"))
            {
                ops[j].type = XML_ELEM;
            }
            else if (!strcmp(argv[i], "attr"))
            {
                ops[j].type = XML_ATTR;
            }
            else if (!strcmp(argv[i], "text"))
            {
                ops[j].type = XML_TEXT;
            }
            else edUsage(argv[0], EXIT_BAD_ARGS);;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            if (strcmp(argv[i], "-n") && strcmp(argv[i], "--name")) edUsage(argv[0], EXIT_BAD_ARGS);;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            ops[j].arg3 = argv[i];
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            if (strcmp(argv[i], "-v") && strcmp(argv[i], "--value")) edUsage(argv[0], EXIT_BAD_ARGS);;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            ops[j].arg2 = argv[i];
            j++;
        }
        else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--append"))
        {
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            ops[j].type = XML_UNDEFINED;
            ops[j].op = XML_ED_APPEND;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            if (strcmp(argv[i], "-t") && strcmp(argv[i], "--type")) edUsage(argv[0], EXIT_BAD_ARGS);;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            if (!strcmp(argv[i], "elem"))
            {
                ops[j].type = XML_ELEM;
            }
            else if (!strcmp(argv[i], "attr"))
            {
                ops[j].type = XML_ATTR;
            }
            else if (!strcmp(argv[i], "text"))
            {
                ops[j].type = XML_TEXT;
            }
            else edUsage(argv[0], EXIT_BAD_ARGS);;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            if (strcmp(argv[i], "-n") && strcmp(argv[i], "--name")) edUsage(argv[0], EXIT_BAD_ARGS);;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            ops[j].arg3 = argv[i];
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            if (strcmp(argv[i], "-v") && strcmp(argv[i], "--value")) edUsage(argv[0], EXIT_BAD_ARGS);;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            ops[j].arg2 = argv[i];
            j++;
        }
        else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--subnode"))
        {
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            ops[j].type = XML_UNDEFINED;
            ops[j].op = XML_ED_SUBNODE;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            if (strcmp(argv[i], "-t") && strcmp(argv[i], "--type")) edUsage(argv[0], EXIT_BAD_ARGS);;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            if (!strcmp(argv[i], "elem"))
            {
                ops[j].type = XML_ELEM;
            }
            else if (!strcmp(argv[i], "attr"))
            {
                ops[j].type = XML_ATTR;
            }
            else if (!strcmp(argv[i], "text"))
            {
                ops[j].type = XML_TEXT;
            }
            else edUsage(argv[0], EXIT_BAD_ARGS);;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            if (strcmp(argv[i], "-n") && strcmp(argv[i], "--name")) edUsage(argv[0], EXIT_BAD_ARGS);;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            ops[j].arg3 = argv[i];
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            if (strcmp(argv[i], "-v") && strcmp(argv[i], "--value")) edUsage(argv[0], EXIT_BAD_ARGS);;
            i++;
            if (i >= argc) edUsage(argv[0], EXIT_BAD_ARGS);;
            ops[j].arg2 = argv[i];
            j++;
        }
        else
        {
            if (argv[i][0] != '-')
            {
                break;
            }
        }

        i++;
    }

    ops_count = j;

    xmlKeepBlanksDefault(0);

    if ((!g_ops.noblanks) || g_ops.preserveFormat) xmlKeepBlanksDefault(1);

    if (i >= argc)
    {
        edOutput("-", g_ops);
    }
    
    for (n=i; n<argc; n++)
    {
        edOutput(argv[n], g_ops);
    }

    cleanupNSArr(ns_arr);
    xmlCleanupParser();
    xmlCleanupGlobals();
    return 0;
}

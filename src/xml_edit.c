/*  $Id: xml_edit.c,v 1.35 2003/11/04 22:43:15 mgrouch Exp $  */

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>
#include <libxml/debugXML.h>
#include <libxml/xmlIO.h>
#include <libxml/HTMLtree.h>
#include <libxml/xinclude.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xpointer.h>
#include <libxml/parserInternals.h>
#include <libxml/uri.h>

/*
   TODO:
          1. Should this be allowed ?
             ./xml ed -m /xml /xml/table/rec/object ../examples/xml/tab-obj.xml 

          2. Other options insert/append/update/rename/subnode (see tree.c)

          3. Code clean-up is needed. (Too much code replication to find nodeset
             by xpath expression).
*/

#define MAX_NS_ARGS    256

typedef struct _edOptions {   /* Global 'edit' options */
    int noblanks;             /* Remove insignificant spaces from XML tree */
    int preserveFormat;       /* Preserve original XML formatting */
    int omit_decl;            /* Ommit XML declaration line <?xml version="1.0"?> */
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

static const char *ns_arr[2 * MAX_NS_ARGS + 1];
int nCount = 0;

int ops_count = 0;
static XmlEdAction ops[MAX_XML_ED_OPS];

static const char edit_usage_str[] =
"XMLStarlet Toolkit: Edit XML document(s)\n"
"Usage: xml ed <global-options> {<action>} [ <xml-file-or-uri> ... ]\n"
"where\n"
"  <global-options>  - global options for editing\n"
"  <xml-file-or-uri> - input XML document file name/uri (stdin is used if missing)\n\n"

"<global-options> are:\n"
"  -P (or --pf)        - preserve original formatting\n"
"  -S (or --ps)        - preserve non-significant spaces\n"
"  -O (or --omit-decl) - omit XML declaration (<?xml ...?>)\n"
"  -N <name>=<value>   - predefine namespaces (name without \'xmlns:\')\n"
"                        ex: xsql=urn:oracle-xsql\n"
"                        Multiple -N options are allowed.\n"
"                        -N options must be last global options.\n"
"  --help or -h        - display help\n\n"

"where <action>\n"
"   -d or --delete <xpath>\n"
"   -i or --insert <xpath> -t (--type) elem|text|attr -n <name> -v (--value) <value>\n"
"   -a or --append <xpath> -t (--type) elem|text|attr -n <name> -v (--value) <value>\n"
"   -s or --subnode <xpath> -t (--type) elem|text|attr -n <name> -v (--value) <value>\n"
"   -m or --move <xpath1> <xpath2>\n"
"   -r or --rename <xpath1> -v <new-name>\n"
"   -u or --update <xpath> -v (--value) <value>\n"
                  "\t\t\t  -x (--expr) <xpath> (-x is not implemented yet)\n\n";

/*
   How to increment value of every attribute @weight?
   How in --update refer to current value?
   How to insert from a file?

   xml ed --update "//elem/@weight" -x "./@weight+1"?
*/

/**
 *  display short help message
 */
void
edUsage(int argc, char **argv)
{
    extern const char more_info[];
    FILE* o = stderr;
    fprintf(o, edit_usage_str);
    fprintf(o, more_info);
    exit(1);
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
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") ||
                 !strcmp(argv[i], "-?") || !strcmp(argv[i], "-Z"))
        {
            edUsage(argc, argv);
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
 *  Parse command line for additional namespaces
 */
int
edParseNSArr(const char** ns_arr, int* plen,
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
                if (i >= count) edUsage(0, NULL);

                for(j=0; argv[i][j] && (argv[i][j] != '='); j++);
                if (argv[i][j] != '=') edUsage(0, NULL);

                name = xmlStrndup((const xmlChar *) argv[i], j);
                value = xmlStrdup((const xmlChar *) argv[i]+j+1);

                if (*plen >= MAX_NS_ARGS)
                {
                    fprintf(stderr, "too many namespaces increase MAX_NS_ARGS\n");
                    exit(2);
                }

                ns_arr[*plen] = (char *)name;
                (*plen)++;
                ns_arr[*plen] = (char *)value;
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
edCleanupNSArr(const char **ns_arr)
{
    const char **p = ns_arr;

    while (*p)
    {
        xmlFree((char *)*p);
        p++;
    }
}

/**
 *  'update' operation
 */
void
edUpdate(xmlDocPtr doc, const char *loc, const char *val, XmlNodeType type)
{
    int xptr = 0;
    int expr = 0;
    int tree = 0;

    xmlXPathObjectPtr res = NULL;
    xmlXPathContextPtr ctxt;

#if defined(LIBXML_XPTR_ENABLED)
    if (xptr)
    {
        ctxt = xmlXPtrNewContext(doc, NULL, NULL);
        res = xmlXPtrEval(BAD_CAST loc, ctxt);
    }
    else
    {
#endif
        int ns = 0;
        ctxt = xmlXPathNewContext(doc);
        while(ns_arr[ns])
        {
            xmlXPathRegisterNs(ctxt, (const xmlChar *) ns_arr[ns], (const xmlChar *) ns_arr[ns+1]);
            ns += 2;
        }

        ctxt->node = xmlDocGetRootElement(doc);
        if (expr)
            res = xmlXPathEvalExpression(BAD_CAST loc, ctxt);
        else
        {
            xmlXPathCompExprPtr comp;

            comp = xmlXPathCompile(BAD_CAST loc);
            if (comp != NULL)
            {
                if (tree)
                    xmlXPathDebugDumpCompExpr(stdout, comp, 0);

                res = xmlXPathCompiledEval(comp, ctxt);
                xmlXPathFreeCompExpr(comp);
            }
            else res = NULL;
        }
#if defined(LIBXML_XPTR_ENABLED)
    }
#endif
    if (res == NULL) return;

    /* Found loc */
    switch(res->type)
    {
        case XPATH_NODESET:
        {
            int i;
            xmlNodeSetPtr cur = res->nodesetval;
            if (cur)
            {
                /*
                fprintf(stderr, "Set contains %d nodes:\n", cur->nodeNr);
                */
                for (i = 0; i < cur->nodeNr; i++)
                {
                    /*
                     *  update node
                     */
                    xmlNodeSetContent(cur->nodeTab[i], BAD_CAST val);
                }
            }
            break;
        }
        default:
            break;
    }
    xmlXPathFreeObject(res);
    xmlXPathFreeContext(ctxt);
}

/**
 *  'insert' operation
 */
void
edInsert(xmlDocPtr doc, const char *loc, const char *val, const char *name,
         XmlNodeType type, int mode)
{
    int xptr = 0;
    int expr = 0;
    int tree = 0;

    xmlXPathObjectPtr res = NULL;
    xmlXPathContextPtr ctxt;

#if defined(LIBXML_XPTR_ENABLED)
    if (xptr)
    {
        ctxt = xmlXPtrNewContext(doc, NULL, NULL);
        res = xmlXPtrEval(BAD_CAST loc, ctxt);
    }
    else
    {
#endif
        int ns = 0;
        ctxt = xmlXPathNewContext(doc);
        while(ns_arr[ns])
        {
            xmlXPathRegisterNs(ctxt, (const xmlChar *) ns_arr[ns], (const xmlChar *) ns_arr[ns+1]);
            ns += 2;
        }

        ctxt->node = xmlDocGetRootElement(doc);
        if (expr)
            res = xmlXPathEvalExpression(BAD_CAST loc, ctxt);
        else
        {
            xmlXPathCompExprPtr comp;

            comp = xmlXPathCompile(BAD_CAST loc);
            if (comp != NULL)
            {
                if (tree)
                    xmlXPathDebugDumpCompExpr(stdout, comp, 0);

                res = xmlXPathCompiledEval(comp, ctxt);
                xmlXPathFreeCompExpr(comp);
            }
            else res = NULL;
        }
#if defined(LIBXML_XPTR_ENABLED)
    }
#endif
    if (res == NULL) return;

    /* Found loc */
    switch(res->type)
    {
        case XPATH_NODESET:
        {
            int i;
            xmlNodeSetPtr cur = res->nodesetval;
            if (cur)
            {
                for (i = 0; i < cur->nodeNr; i++)
                {
                    /*
                     *  update node
                     */
                    if (type == XML_ATTR)
                    {
                        xmlNewProp(cur->nodeTab[i], BAD_CAST name, BAD_CAST val);
                    }
                    else if (type == XML_ELEM)
                    {
                        xmlNodePtr node = xmlNewDocNode(doc, NULL /* TODO: NS */, BAD_CAST name, BAD_CAST val);
                        if (mode > 0)
                            xmlAddNextSibling(cur->nodeTab[i], node);
                        else if (mode < 0)
                            xmlAddPrevSibling(cur->nodeTab[i], node);
                        else
                            xmlAddChild(cur->nodeTab[i], node);
                    }
                    else if (type == XML_TEXT)
                    {
                        xmlNodePtr node = xmlNewDocText(doc, BAD_CAST val);
                        if (mode > 0)
                            xmlAddNextSibling(cur->nodeTab[i], node);
                        else if (mode < 0)
                            xmlAddPrevSibling(cur->nodeTab[i], node);
                        else
                            xmlAddChild(cur->nodeTab[i], node);                    
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    xmlXPathFreeObject(res);
    xmlXPathFreeContext(ctxt);
}

/**
 *  'rename' operation
 */
void
edRename(xmlDocPtr doc, char *loc, char *val, XmlNodeType type)
{
    int xptr = 0;
    int expr = 0;
    int tree = 0;

    xmlXPathObjectPtr res = NULL;
    xmlXPathContextPtr ctxt;

#if defined(LIBXML_XPTR_ENABLED)
    if (xptr)
    {
        ctxt = xmlXPtrNewContext(doc, NULL, NULL);
        res = xmlXPtrEval(BAD_CAST loc, ctxt);
    }
    else
    {
#endif
        int ns = 0;
        ctxt = xmlXPathNewContext(doc);
        while(ns_arr[ns])
        {
            xmlXPathRegisterNs(ctxt, (const xmlChar *) ns_arr[ns], (const xmlChar *) ns_arr[ns+1]);
            ns += 2;
        }

        ctxt->node = xmlDocGetRootElement(doc);
        if (expr)
            res = xmlXPathEvalExpression(BAD_CAST loc, ctxt);
        else
        {
            xmlXPathCompExprPtr comp;

            comp = xmlXPathCompile(BAD_CAST loc);
            if (comp != NULL)
            {
                if (tree)
                    xmlXPathDebugDumpCompExpr(stdout, comp, 0);

                res = xmlXPathCompiledEval(comp, ctxt);
                xmlXPathFreeCompExpr(comp);
            }
            else res = NULL;
        }
#if defined(LIBXML_XPTR_ENABLED)
    }
#endif
    if (res == NULL) return;

    /* Found loc */    
    switch(res->type)
    {
        case XPATH_NODESET:
        {
            int i;
            xmlNodeSetPtr cur = res->nodesetval;
            if (cur)
            {
                /*
                fprintf(stderr, "Set contains %d nodes:\n", cur->nodeNr);
                */
                for (i = 0; i < cur->nodeNr; i++)
                {
                    /*
                     *  rename node
                     */
                    if (cur->nodeTab[i]->type == XML_ATTRIBUTE_NODE)
                    {
                        xmlFree((xmlChar *) cur->nodeTab[i]->name);
                        cur->nodeTab[i]->name = xmlStrdup((const xmlChar*) val);
                    }
                    else
                    {
                        xmlFree((xmlChar *)cur->nodeTab[i]->name);
                        cur->nodeTab[i]->name = xmlStrdup((const xmlChar*) val);
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    xmlXPathFreeObject(res);
    xmlXPathFreeContext(ctxt);
}

/**
 *  'delete' operation
 */
void
edDelete(xmlDocPtr doc, char *str)
{
    int xptr = 0;
    int expr = 0;
    int tree = 0;

    xmlXPathObjectPtr res = NULL;
    xmlXPathContextPtr ctxt;

#if defined(LIBXML_XPTR_ENABLED)
    if (xptr)
    {
        ctxt = xmlXPtrNewContext(doc, NULL, NULL);
        res = xmlXPtrEval(BAD_CAST str, ctxt);
    }
    else
    {
#endif
        int ns = 0;
        ctxt = xmlXPathNewContext(doc);
        while(ns_arr[ns])
        {
            xmlXPathRegisterNs(ctxt, (const xmlChar *) ns_arr[ns], (const xmlChar *) ns_arr[ns+1]);
            ns += 2;
        }
        
        ctxt->node = xmlDocGetRootElement(doc);
        if (expr)
            res = xmlXPathEvalExpression(BAD_CAST str, ctxt);
        else
        {
            xmlXPathCompExprPtr comp;

            comp = xmlXPathCompile(BAD_CAST str);
            if (comp != NULL)
            {
                if (tree)
                    xmlXPathDebugDumpCompExpr(stdout, comp, 0);

                res = xmlXPathCompiledEval(comp, ctxt);
                xmlXPathFreeCompExpr(comp);
            }
            else res = NULL;
        }
#if defined(LIBXML_XPTR_ENABLED)
    }
#endif
    if (res == NULL) return;

    switch(res->type)
    {
        case XPATH_NODESET:
        {
            int i;
            xmlNodeSetPtr cur = res->nodesetval;
            if (cur)
            {
                /*
                fprintf(stderr, "Set contains %d nodes:\n", cur->nodeNr);
                */
                for (i = 0; i < cur->nodeNr; i++)
                {
                    /*
                     *  delete node
                     */
                    xmlUnlinkNode(cur->nodeTab[i]);
                    /* xmlFreeNode(cur->nodeTab[i]); ??? */
                }
            }
            break;
        }
        default:
            break;
    }
    xmlXPathFreeObject(res);
    xmlXPathFreeContext(ctxt);
}

/**
 *  'move' operation
 */
void
edMove(xmlDocPtr doc, char *from, char *to)
{
    int xptr = 0;
    int expr = 0;
    int tree = 0;

    xmlXPathObjectPtr res = NULL, res_to = NULL;
    xmlXPathContextPtr ctxt;

    /*
     *  Find 'from' node set
     */
#if defined(LIBXML_XPTR_ENABLED)
    if (xptr)
    {
        ctxt = xmlXPtrNewContext(doc, NULL, NULL);
        res = xmlXPtrEval(BAD_CAST from, ctxt);
    }
    else
    {
#endif
        int ns = 0;
        ctxt = xmlXPathNewContext(doc);
        while(ns_arr[ns])
        {
            xmlXPathRegisterNs(ctxt, (const xmlChar *) ns_arr[ns], (const xmlChar *) ns_arr[ns+1]);
            ns += 2;
        }

        ctxt->node = xmlDocGetRootElement(doc);
        if (expr)
            res = xmlXPathEvalExpression(BAD_CAST from, ctxt);
        else
        {
            xmlXPathCompExprPtr comp;

            comp = xmlXPathCompile(BAD_CAST from);
            if (comp != NULL)
            {
                if (tree)
                    xmlXPathDebugDumpCompExpr(stdout, comp, 0);

                res = xmlXPathCompiledEval(comp, ctxt);
                xmlXPathFreeCompExpr(comp);
            }
            else res = NULL;
        }
#if defined(LIBXML_XPTR_ENABLED)
    }
#endif
    if (res == NULL) return;

    /********************************************************/
    
    /*
     *  Find 'to' node set
     */
#if defined(LIBXML_XPTR_ENABLED)
    if (xptr)
    {
        ctxt = xmlXPtrNewContext(doc, NULL, NULL);
        res_to = xmlXPtrEval(BAD_CAST to, ctxt);
    }
    else
    {
#endif
        int ns = 0;
        ctxt = xmlXPathNewContext(doc);
        while(ns_arr[ns])
        {
            xmlXPathRegisterNs(ctxt, (const xmlChar *) ns_arr[ns], (const xmlChar *) ns_arr[ns+1]);
            ns += 2;
        }

        ctxt->node = xmlDocGetRootElement(doc);
        if (expr)
            res_to = xmlXPathEvalExpression(BAD_CAST to, ctxt);
        else
        {
            xmlXPathCompExprPtr comp;

            comp = xmlXPathCompile(BAD_CAST to);
            if (comp != NULL)
            {
                if (tree)
                    xmlXPathDebugDumpCompExpr(stdout, comp, 0);

                res_to = xmlXPathCompiledEval(comp, ctxt);
                xmlXPathFreeCompExpr(comp);
            }
            else
                res_to = NULL;
        }
#if defined(LIBXML_XPTR_ENABLED)
    }
#endif
    if (res_to == NULL) return;

    switch(res_to->type)
    {
        case XPATH_NODESET:
        {
            xmlNodeSetPtr cur = res_to->nodesetval;
            if (cur)
            {
                if (cur->nodeNr != 1)
                {
                    fprintf(stderr, "destination nodeset does not contain one node (node count is %d)\n", cur->nodeNr);
                    break;
                }
            }
        }
        default:
            break;
    }

    switch(res->type)
    {
        case XPATH_NODESET:
        {
            int i;
            xmlNodeSetPtr cur = res->nodesetval;
            if (cur && (res_to->nodesetval) && (res_to->nodesetval->nodeNr != 1))
            {
                for (i = 0; i < cur->nodeNr; i++)
                {
                    /*
                     *  delete node
                     */
                    xmlUnlinkNode(cur->nodeTab[i]);
                    xmlAddChild(res_to->nodesetval->nodeTab[0], cur->nodeTab[i]);             
                }
            }
            break;
        }
        default:
            break;
    }

    xmlXPathFreeObject(res);
    xmlXPathFreeObject(res_to);
    xmlXPathFreeContext(ctxt);
}

/**
 *  Loop through array of operations and perform them
 */
int
edProcess(xmlDocPtr doc, XmlEdAction* ops, int ops_count)
{
    int res = 0;
    int k;
    
    for (k = 0; k < ops_count; k++)
    {
        switch (ops[k].op)
        {
            case XML_ED_DELETE:
                edDelete(doc, ops[k].arg1);
                break;
            case XML_ED_MOVE:
                edMove(doc, ops[k].arg1, ops[k].arg2);
                break;
            case XML_ED_UPDATE:
                edUpdate(doc, ops[k].arg1, ops[k].arg2, ops[k].type);
                break;
            case XML_ED_RENAME:
                edRename(doc, ops[k].arg1, ops[k].arg2, ops[k].type);
                break;
            case XML_ED_INSERT:
                edInsert(doc, ops[k].arg1, ops[k].arg2, ops[k].arg3, ops[k].type, -1);
                break;
            case XML_ED_APPEND:
                edInsert(doc, ops[k].arg1, ops[k].arg2, ops[k].arg3, ops[k].type, 1);
                break;
            case XML_ED_SUBNODE:
                edInsert(doc, ops[k].arg1, ops[k].arg2, ops[k].arg3, ops[k].type, 0);
                break;
            default:
                break;
        }
    }

    return res;
}

/**
 *  This is the main function for 'edit' option
 */
int
edMain(int argc, char **argv)
{
    int i, j, n, start = 0;
    static edOptions g_ops;

    if (argc < 3) edUsage(argc, argv);
    if (!strcmp(argv[2], "--help") || !strcmp(argv[2], "-h")) edUsage(argc, argv);

    edInitOptions(&g_ops);
    start = edParseOptions(&g_ops, argc, argv);

    edParseNSArr(ns_arr, &nCount, start, argv+start);
        
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
            if (i >= argc) edUsage(argc, argv);
            ops[j].op = XML_ED_DELETE;
            ops[j].arg1 = argv[i];
            ops[j].arg2 = 0;
            ops[j].type = XML_UNDEFINED;
            j++;            
        }
        else if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--move"))
        {
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].op = XML_ED_MOVE;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].arg2 = argv[i];
            ops[j].type = XML_UNDEFINED;
            j++;
        }
        else if (!strcmp(argv[i], "-u") || !strcmp(argv[i], "--update"))
        {
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].type = XML_UNDEFINED;
            ops[j].op = XML_ED_UPDATE;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edUsage(argc, argv);
            if (strcmp(argv[i], "-v") && strcmp(argv[i], "--value") &&
                strcmp(argv[i], "-x") && strcmp(argv[i], "--expr")) edUsage(argc, argv);

            if (!strcmp(argv[i], "-x") || !strcmp(argv[i], "--expr"))
                ops[j].type = XML_EXPR;
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].arg2 = argv[i];

            j++;
        }
        else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--rename"))
        {
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].type = XML_UNDEFINED;
            ops[j].op = XML_ED_RENAME;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edUsage(argc, argv);
            if (strcmp(argv[i], "-v") && strcmp(argv[i], "--value")) edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].arg2 = argv[i];
            j++;
        }
        else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--insert"))
        {
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].type = XML_UNDEFINED;
            ops[j].op = XML_ED_INSERT;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edUsage(argc, argv);
            if (strcmp(argv[i], "-t") && strcmp(argv[i], "--type")) edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
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
            else edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
            if (strcmp(argv[i], "-n") && strcmp(argv[i], "--name")) edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].arg3 = argv[i];
            i++;
            if (i >= argc) edUsage(argc, argv);
            if (strcmp(argv[i], "-v") && strcmp(argv[i], "--value")) edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].arg2 = argv[i];
            j++;
        }
        else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--append"))
        {
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].type = XML_UNDEFINED;
            ops[j].op = XML_ED_APPEND;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edUsage(argc, argv);
            if (strcmp(argv[i], "-t") && strcmp(argv[i], "--type")) edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
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
            else edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
            if (strcmp(argv[i], "-n") && strcmp(argv[i], "--name")) edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].arg3 = argv[i];
            i++;
            if (i >= argc) edUsage(argc, argv);
            if (strcmp(argv[i], "-v") && strcmp(argv[i], "--value")) edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].arg2 = argv[i];
            j++;
        }
        else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--subnode"))
        {
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].type = XML_UNDEFINED;
            ops[j].op = XML_ED_SUBNODE;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edUsage(argc, argv);
            if (strcmp(argv[i], "-t") && strcmp(argv[i], "--type")) edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
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
            else edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
            if (strcmp(argv[i], "-n") && strcmp(argv[i], "--name")) edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
            ops[j].arg3 = argv[i];
            i++;
            if (i >= argc) edUsage(argc, argv);
            if (strcmp(argv[i], "-v") && strcmp(argv[i], "--value")) edUsage(argc, argv);
            i++;
            if (i >= argc) edUsage(argc, argv);
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
        xmlDocPtr doc = xmlParseFile("-");

        if (!doc) exit(2);

        edProcess(doc, ops, ops_count);

        /* Print out result */
        if (!g_ops.omit_decl)
        {
            xmlSaveFormatFile("-", doc, 1);
        }
        else
        {
            int format = 1;
            int ret = 0;
            char *encoding = NULL;
            xmlOutputBufferPtr buf = NULL;
            xmlCharEncodingHandlerPtr handler = NULL;
            buf = xmlOutputBufferCreateFile(stdout, handler);

            if (doc->children != NULL)
            {
               xmlNodePtr child = doc->children;
               while (child != NULL)
               {
                  xmlNodeDumpOutput(buf, doc, child, 0, format, encoding);
                  xmlOutputBufferWriteString(buf, "\n");
                  child = child->next;
               }
            }
            ret = xmlOutputBufferClose(buf);
        }
    }
    
    for (n=i; n<argc; n++)
    {
        xmlDocPtr doc = xmlParseFile(argv[n]);

        if (!doc) exit(2);

        edProcess(doc, ops, ops_count);

        /* Print out result */
        if (!g_ops.omit_decl)
        {
            xmlSaveFormatFile("-", doc, 1);
        }
        else
        {
            int format = 1;
            int ret = 0;
            char *encoding = NULL;
            xmlOutputBufferPtr buf = NULL;
            xmlCharEncodingHandlerPtr handler = NULL;
            buf = xmlOutputBufferCreateFile(stdout, handler);

            if (doc->children != NULL)
            {
               xmlNodePtr child = doc->children;
               while (child != NULL)
               {
                  xmlNodeDumpOutput(buf, doc, child, 0, format, encoding);
                  xmlOutputBufferWriteString(buf, "\n");
                  child = child->next;
               }
            }
            ret = xmlOutputBufferClose(buf);
        }
    }

    edCleanupNSArr(ns_arr);
    return 0;
}

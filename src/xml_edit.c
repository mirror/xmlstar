/*  $Id: xml_edit.c,v 1.25 2003/05/28 19:04:39 mgrouch Exp $  */

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
XmlEdAction ops[MAX_XML_ED_OPS];

static const char edit_usage_str[] =
"XMLStarlet Toolkit: Edit XML document(s)\n"
"Usage: xml ed {<action>} [ <xml-file-or-uri> ... ]\n"
"where <action>\n"
"   -d or --delete <xpath>\n"
"   -i or --insert <xpath> -t (--type) elem|text|attr -n <name> -v (--value) <value>\n"
"   -a or --append <xpath> -t (--type) elem|text|attr -n <name> -v (--value) <value>\n"
"   -s or --subnode <xpath> -t (--type) elem|text|attr -n <name> -v (--value) <value>\n"
"   -m or --move <xpath1> <xpath2>\n"
"   -r or --rename <xpath1> -v <new-name>\n"
"   -u or --update <xpath> -v (--value) <value>\n"
                  "\t\t\t  -x (--expr) <xpath>\n\n"
"Currently only --delete, --move, and --rename are implemented.\n\n";

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
 *  'update' operation
 */
void
edUpdate(xmlDocPtr doc, const char *loc, const char *val, XmlNodeType type)
{
    int xptr = 0;
    int expr = 0;
    int tree = 0;

    xmlXPathObjectPtr res;
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
        ctxt = xmlXPathNewContext(doc);
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
         XmlNodeType type, int append)
{
    int xptr = 0;
    int expr = 0;
    int tree = 0;

    xmlXPathObjectPtr res;
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
        ctxt = xmlXPathNewContext(doc);
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
                    if (append)
                        xmlAddNextSibling(cur->nodeTab[i], node);
                    else                    
                        xmlAddPrevSibling(cur->nodeTab[i], node);
                }
                else if (type == XML_TEXT)
                {
                    xmlNodePtr node = xmlNewDocText(doc, BAD_CAST val);
                    if (append)
                        xmlAddNextSibling(cur->nodeTab[i], node);
                    else
                        xmlAddPrevSibling(cur->nodeTab[i], node);
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

    xmlXPathObjectPtr res;
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
        ctxt = xmlXPathNewContext(doc);
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

    xmlXPathObjectPtr res;
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
        ctxt = xmlXPathNewContext(doc);
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

    xmlXPathObjectPtr res, res_to;
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
        ctxt = xmlXPathNewContext(doc);
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
        ctxt = xmlXPathNewContext(doc);
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
            if (cur->nodeNr != 1)
            {
                fprintf(stderr, "destination nodeset does not contain one node (node count is %d)\n", cur->nodeNr);
                break;
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
            for (i = 0; i < cur->nodeNr; i++)
            {
                /*
                 *  delete node
                 */
                xmlUnlinkNode(cur->nodeTab[i]);
                xmlAddChild(res_to->nodesetval->nodeTab[0], cur->nodeTab[i]);             
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
                edInsert(doc, ops[k].arg1, ops[k].arg2, ops[k].arg3, ops[k].type, 0);
                break;
            case XML_ED_APPEND:
                edInsert(doc, ops[k].arg1, ops[k].arg2, ops[k].arg3, ops[k].type, 1);
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
    int i, j, n;

    if (argc < 3) edUsage(argc, argv);
    if (!strcmp(argv[2], "--help")) edUsage(argc, argv);
    
    /*
     *  Parse command line and fill array of operations
     */
    j = 0;
    i = 2;
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

    if (i >= argc)
    {
        xmlDocPtr doc = xmlParseFile("-");
        edProcess(doc, ops, ops_count);
        xmlSaveFormatFile("-", doc, 1);
    }
    
    for (n=i; n<argc; n++)
    {
        xmlDocPtr doc = xmlParseFile(argv[n]);
        edProcess(doc, ops, ops_count);
        xmlSaveFormatFile("-", doc, 1);
    }

    return 0;
}

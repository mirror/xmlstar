/*  $Id: xml_edit.c,v 1.17 2003/04/19 00:27:09 mgrouch Exp $  */

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

          2. Other options insert/append/update/rename/subnode
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
   XML_CDATA
} XmlNodeType;

typedef char* XmlEdArg;


typedef struct _XmlEdAction {
  XmlEdOp       op;
  XmlEdArg      arg1;
  XmlEdArg      arg2;
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
"   -i or --insert <xpath> -t (--type) elem|text|attr -v (--value) <value>\n"
"   -a or --append <xpath> -t (--type) elem|text|attr -v (--value) <value>\n"
"   -s or --subnode <xpath> -t (--type) elem|text|attr -v (--value) <value>\n"
"   -m or --move <xpath1> <xpath2>\n"
"   -r or --rename <xpath1> -v <new-name>\n"
"   -u or --update <xpath> -v (--value) <value>\n"
                  "\t\t\t  -x (--expr) <xpath>\n\n";

/*
   How to increment value of every attribute @weight?
   How in --update refer to current value?
   How to insert from a file?

   xml ed --update "//elem/@weight" -x "./@weight+1"?
*/

#if 0
void
xml_XPathDebugDumpObject(FILE *output, xmlXPathObjectPtr cur, int depth) {
    int i;
    char shift[100];

    for (i = 0;((i < depth) && (i < 25));i++)
        shift[2 * i] = shift[2 * i + 1] = ' ';
    shift[2 * i] = shift[2 * i + 1] = 0;

    fprintf(output, shift);

    if (cur == NULL) {
        fprintf(output, "Object is empty (NULL)\n");
	return;
    }
    switch(cur->type) {
        case XPATH_UNDEFINED:
	    fprintf(output, "Object is uninitialized\n");
	    break;
        case XPATH_NODESET:
	    fprintf(output, "Object is a Node Set :\n");
	    xmlXPathDebugDumpNodeSet(output, cur->nodesetval, depth);
	    break;
	case XPATH_XSLT_TREE:
	    fprintf(output, "Object is an XSLT value tree :\n");
	    xmlXPathDebugDumpValueTree(output, cur->nodesetval, depth);
	    break;
        case XPATH_BOOLEAN:
	    fprintf(output, "Object is a Boolean : ");
	    if (cur->boolval) fprintf(output, "true\n");
	    else fprintf(output, "false\n");
	    break;
        case XPATH_NUMBER:
	    switch (xmlXPathIsInf(cur->floatval)) {
	    case 1:
		fprintf(output, "Object is a number : Infinity\n");
		break;
	    case -1:
		fprintf(output, "Object is a number : -Infinity\n");
		break;
	    default:
		if (xmlXPathIsNaN(cur->floatval)) {
		    fprintf(output, "Object is a number : NaN\n");
		} else if (cur->floatval == 0 && xmlXPathGetSign(cur->floatval) != 0) {
		    fprintf(output, "Object is a number : 0\n");
		} else {
		    fprintf(output, "Object is a number : %0g\n", cur->floatval);
		}
	    }
	    break;
        case XPATH_STRING:
	    fprintf(output, "Object is a string : ");
	    xmlDebugDumpString(output, cur->stringval);
	    fprintf(output, "\n");
	    break;
	case XPATH_POINT:
	    fprintf(output, "Object is a point : index %d in node", cur->index);
	    xmlXPathDebugDumpNode(output, (xmlNodePtr) cur->user, depth + 1);
	    fprintf(output, "\n");
	    break;
	case XPATH_RANGE:
	    if ((cur->user2 == NULL) ||
		((cur->user2 == cur->user) && (cur->index == cur->index2))) {
		fprintf(output, "Object is a collapsed range :\n");
		fprintf(output, shift);
		if (cur->index >= 0)
		    fprintf(output, "index %d in ", cur->index);
		fprintf(output, "node\n");
		xmlXPathDebugDumpNode(output, (xmlNodePtr) cur->user,
			              depth + 1);
	    } else  {
		fprintf(output, "Object is a range :\n");
		fprintf(output, shift);
		fprintf(output, "From ");
		if (cur->index >= 0)
		    fprintf(output, "index %d in ", cur->index);
		fprintf(output, "node\n");
		xmlXPathDebugDumpNode(output, (xmlNodePtr) cur->user,
			              depth + 1);
		fprintf(output, shift);
		fprintf(output, "To ");
		if (cur->index2 >= 0)
		    fprintf(output, "index %d in ", cur->index2);
		fprintf(output, "node\n");
		xmlXPathDebugDumpNode(output, (xmlNodePtr) cur->user2,
			              depth + 1);
		fprintf(output, "\n");
	    }
	    break;
	case XPATH_LOCATIONSET:
#if defined(LIBXML_XPTR_ENABLED)
	    fprintf(output, "Object is a Location Set:\n");
	    xmlXPathDebugDumpLocationSet(output,
		    (xmlLocationSetPtr) cur->user, depth);
#endif
	    break;
	case XPATH_USERS:
	    fprintf(output, "Object is user defined\n");
	    break;
    }
}
#endif

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
            /* res = xmlXPathEval(BAD_CAST str, ctxt); */
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
            fprintf(stderr, "Object is a Node Set :\n");
            fprintf(stderr, "Set contains %d nodes:\n", cur->nodeNr);
            */
            for (i = 0; i < cur->nodeNr; i++)
            {
                /*
                fprintf(output, shift);
                fprintf(output, "%d", i + 1);
                xmlXPathDebugDumpNode(output, cur->nodeTab[i], depth + 1);
                */

                /*
                 *  delete node
                 */
                 xmlUnlinkNode(cur->nodeTab[i]);
                 /*fprintf(stderr, "unlinked\n");*/
                 /*xmlFreeNode(cur->nodeTab[i]);*/
            }
            /*
            xmlXPathDebugDumpNodeSet(output, cur->nodesetval, depth);
            */
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
/*
fprintf(stderr, "(node count is %d)\n", cur->nodeNr);
*/            
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
/*
fprintf(stderr, "Object is a Node Set :\n");
fprintf(stderr, "Set contains %d nodes:\n", cur->nodeNr);
*/
            for (i = 0; i < cur->nodeNr; i++)
            {
                /*
                fprintf(output, shift);
                fprintf(output, "%d", i + 1);
                xmlXPathDebugDumpNode(output, cur->nodeTab[i], depth + 1);
                */

                /*
                 *  delete node
                 */
                xmlUnlinkNode(cur->nodeTab[i]);
/*
fprintf(stderr, "unlinked\n");
*/
                xmlAddChild(res_to->nodesetval->nodeTab[0], cur->nodeTab[i]);
             
                 /*xmlFreeNode(cur->nodeTab[i]);*/
            }
            /*
            xmlXPathDebugDumpNodeSet(output, cur->nodesetval, depth);
            */
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
        /*xmlSaveFile("-", doc);*/
    }
    
    for (n=i; n<argc; n++)
    {
        xmlDocPtr doc = xmlParseFile(argv[n]);
        edProcess(doc, ops, ops_count);
        xmlSaveFormatFile("-", doc, 1);
        /*xmlSaveFile("-", doc);*/
        /* xmlDocFormatDump(stdout, doc, 0); */
        /* xmlDocDump(stdout, doc); */
    }

    return 0;
}

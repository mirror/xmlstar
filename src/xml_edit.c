/* $Id: xml_edit.c,v 1.4 2002/11/23 05:39:57 mgrouch Exp $ */

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


/*
   TODO:  
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
"Usage: xml ed {<action>} [ <xml-file> ... ]\n"
"where <action>\n"
"   -d or --delete <xpath>\n"
"   -i or --insert <xpath> -t (--type) elem|text|attr -v (--value) <value>\n"
"   -a or --append <xpath> -t (--type) elem|text|attr -v (--value) <value>\n"
"   -s or --subnode <xpath> -t (--type) elem|text|attr -v (--value) <value>\n"
"   -m or --move <xpath1> <xpath2>\n"
"   -r or --rename <xpath1> -v <new-name>\n"
"   -u or --update <xpath> -v (--value) <value>\n"
"                          -x (--expr) <xpath>\n\n"

"XMLStarlet is a command line toolkit to query/edit/check/transform\n"
"XML documents (for more information see http://xmlstar.sourceforge.net/)\n";

/*
   How to increment value of every attribute @weight?
   How in --update refer to current value?
   How to insert from a file?

   xml ed --update "//*@weight" -x "./@weight+1"?
*/

void edit_usage(int argc, char **argv)
{
    FILE* o = stderr;
    fprintf(o, edit_usage_str);
    exit(1);
}

int xml_ed_process(xmlDocPtr doc, XmlEdAction* ops, int ops_count)
{
    int k;
    
    for (k = 0; k < ops_count; k++)
    {
        switch (ops[k].op)
        {
            case XML_ED_DELETE:
                //xml_ed_delete();
                break;
            default:
                break;
        }
    }
}

int xml_edit(int argc, char **argv)
{
    int i, j, n;

    if (argc < 3) edit_usage(argc, argv);
    
    j = 0;
    i = 2;
    while (i < argc)
    {
        if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--delete"))
        {
            i++;
            if (i >= argc) edit_usage(argc, argv);
            ops[j].op = XML_ED_DELETE;
            ops[j].arg1 = argv[i];
            ops[j].arg1 = 0;
            ops[j].type = XML_UNDEFINED;
            j++;            
        }
        else if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--move"))
        {
            i++;
            if (i >= argc) edit_usage(argc, argv);
            ops[j].op = XML_ED_MOVE;
            ops[j].arg1 = argv[i];
            i++;
            if (i >= argc) edit_usage(argc, argv);
            ops[j].arg1 = argv[i];
            ops[j].type = XML_UNDEFINED;
            j++;
        }
        else if (argv[i][0] != '-')
        {
            break;
        }

        i++;
    }

    ops_count = j;    

    for (n=i; n<argc; n++)
    {
        xmlDocPtr doc = xmlParseFile(argv[n]);
        xml_ed_process(doc, ops, ops_count);
    }
   
    return 0;
}

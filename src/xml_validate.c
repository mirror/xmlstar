/*  $Id: xml_validate.c,v 1.15 2003/02/19 04:01:26 mgrouch Exp $  */

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

#include "trans.h"

#ifdef LIBXML_SCHEMAS_ENABLED
#include <libxml/xmlschemas.h>
#include <libxml/xmlschemastypes.h>
#endif

/*
 *   TODO: Use cases
 *   1. find malfomed XML documents in a given set of XML files 
 *   2. find XML documents which do not match DTD/XSD in a given set of XML files
 *   3. precompile DTD once
 */

typedef struct _valOptions {
    char *dtd;                /* External DTD URL or file name */
    char *schema;             /* External Schema URL or file name */
    int   err;                /* Allow stderr messages */
    int   wellFormed;         /* Check if well formed only */
    int   listGood;           /* >0 list good, <0 list bad */
} valOptions;

typedef valOptions *valOptionsPtr;

static const char validate_usage_str[] =
"XMLStarlet Toolkit: Validate XML document(s)\n"
"Usage: xml val <options> [ <xml-file> ... ]\n"
"where <options>\n"
"   -d or --dtd <dtd-file>  - validate against DTD\n"
#ifdef LIBXML_SCHEMAS_ENABLED
"   -s or --xsd <xsd-file>  - validate against schema\n"
#endif
"   -x or --xml-out         - print result as xml\n"
"   -e or --err             - print verbose error messages on stderr\n"
"   -b or --list-bad        - list only files which do not validate (default)\n"
"   -g or --list-good       - list only files which validate\n"
"   -n or --none            - do not list files (return result code only)\n"
"   -w or --well-formed     - check only if XML is well-formed (default)\n\n";

/**
 *  display short help message
 */
void
valUsage(int argc, char **argv)
{
    extern const char more_info[];
    FILE* o = stderr;
    fprintf(o, validate_usage_str);
    fprintf(o, more_info);
    exit(1);
}

/**
 *  Initialize global command line options
 */
void
valInitOptions(valOptionsPtr ops)
{
    ops->wellFormed = 1;
    ops->listGood = -1;
    ops->err = 0;
    ops->dtd = 0;
    ops->schema = 0;
}

/**
 *  Parse global command line options
 */
int
valParseOptions(valOptionsPtr ops, int argc, char **argv)
{
    int i;

    i = 2;
    while(i < argc)
    {
        if (!strcmp(argv[i], "--well-formed") || !strcmp(argv[i], "-w"))
        {
            ops->wellFormed = 1;
            i++;
        }
        if (!strcmp(argv[i], "--err") || !strcmp(argv[i], "-e"))
        {
            ops->err = 1;
            i++;
        }
        if (!strcmp(argv[i], "--list-good") || !strcmp(argv[i], "-g"))
        {
            ops->listGood = 1;
            i++;
        }
        if (!strcmp(argv[i], "--list-bad") || !strcmp(argv[i], "-b"))
        {
            ops->listGood = -1;
            i++;
        }
        if (!strcmp(argv[i], "--none") || !strcmp(argv[i], "-n"))
        {
            ops->listGood = 0;
            i++;
        }
        if (!strcmp(argv[i], "--dtd") || !strcmp(argv[i], "-d"))
        {
            i++;
            if (i >= argc) valUsage(argc, argv);
            ops->dtd = argv[i];
            i++;
        }
        if (!strcmp(argv[i], "--schema") || !strcmp(argv[i], "-s"))
        {
            i++;
            if (i >= argc) valUsage(argc, argv);
            ops->schema = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
        {
            valUsage(argc, argv);
        }
        else if (!strcmp(argv[i], "-"))
        {
            i++;
            break;
        }
        else if (argv[i][0] == '-')
        {
            valUsage(argc, argv);
        }
        else
        {
            i++;
            break;
        }
    }

    return i-1;
}

/**
 *  Validate XML document against DTD
 */
int
valAgainstDtd(valOptionsPtr ops, char* dtdvalid, xmlDocPtr doc, char* filename)
{
    int result = 0;

    if (dtdvalid != NULL)
    {
        xmlDtdPtr dtd;

        dtd = xmlParseDTD(NULL, (const xmlChar *)dtdvalid);
        if (dtd == NULL)
        {
            xmlGenericError(xmlGenericErrorContext,
            "Could not parse DTD %s\n", dtdvalid);
            result = 2;
        }
        else
        {
            xmlValidCtxt cvp;
            if (ops->err)
            {
                cvp.userData = (void *) stderr;
                cvp.error    = (xmlValidityErrorFunc) fprintf;
                cvp.warning  = (xmlValidityWarningFunc) fprintf;
            }
            else
            {
                cvp.userData = (void *) NULL;
                cvp.error    = (xmlValidityErrorFunc) NULL;
                cvp.warning  = (xmlValidityWarningFunc) NULL;
            }
                        
            if (!xmlValidateDtd(&cvp, doc, dtd))
            {
                if (ops->listGood < 0)
                {
                    fprintf(stdout, "%s\n", filename);
                }
                else if (ops->listGood == 0)
                    xmlGenericError(xmlGenericErrorContext,
                                    "%s: does not match %s\n",
                                    filename, dtdvalid);
                result = 3;
            }
            else
            {
                if (ops->listGood > 0)
                {
                    fprintf(stdout, "%s\n", filename);
                }
            }
            xmlFreeDtd(dtd);
        }
    }

    return result;
}

/**
 *  This is the main function for 'validate' option
 */
int
valMain(int argc, char **argv)
{
    int start;
    static valOptions ops;
    int invalidFound = 0;
    
    if (argc <=2) valUsage(argc, argv);
    valInitOptions(&ops);
    start = valParseOptions(&ops, argc, argv);

    xmlLineNumbersDefault(1);

    if (ops.dtd)
    {
        int i;

        for (i=start; i<argc; i++)
        {
            xmlDocPtr doc;
            int ret;

            ret = 0;
            doc = NULL;

            if (!ops.err)
            {
                xmlDefaultSAXHandlerInit();
                xmlDefaultSAXHandler.error = NULL;
                xmlDefaultSAXHandler.warning = NULL;
            }

            doc = xmlParseFile(argv[i]);
            if (doc)
            {
                /* TODO: precompile DTD once */                
                ret = valAgainstDtd(&ops, ops.dtd, doc, argv[i]);
                xmlFreeDoc(doc);
            }
            else
            {
                ret = 1; /* Malformed XML or could not open file */
                if (ops.listGood < 0)
                {
                    fprintf(stdout, "%s\n", argv[i]);
                }
            }
            if (ret) invalidFound = 1;     
        }
    }
#ifdef LIBXML_SCHEMAS_ENABLED
    else if (ops.schema)
    {
        xmlSchemaPtr schema = NULL;
        xmlSchemaParserCtxtPtr ctxt = NULL;
        xmlSchemaValidCtxtPtr ctxt2 = NULL;
        int i;

        /* TODO: Do not print debug stuff */
        ctxt = xmlSchemaNewParserCtxt(ops.schema);
        xmlSchemaSetParserErrors(ctxt,
            (xmlSchemaValidityErrorFunc) fprintf,
            (xmlSchemaValidityWarningFunc) fprintf,
            stderr);
               
        schema = xmlSchemaParse(ctxt);
        xmlSchemaFreeParserCtxt(ctxt);

        ctxt2 = xmlSchemaNewValidCtxt(schema);
        xmlSchemaSetValidErrors(ctxt2,
            (xmlSchemaValidityErrorFunc) fprintf,
            (xmlSchemaValidityWarningFunc) fprintf,
            stderr);
        
        for (i=start; i<argc; i++)
        {
            xmlDocPtr doc;
            int ret;

            ret = 0;
            doc = NULL;

            if (!ops.err)
            {
                xmlDefaultSAXHandlerInit();
                xmlDefaultSAXHandler.error = NULL;
                xmlDefaultSAXHandler.warning = NULL;
            }

            doc = xmlParseFile(argv[i]);
            if (doc)
            {
                ret = xmlSchemaValidateDoc(ctxt2, doc);
                xmlFreeDoc(doc);
            }
            else
            {
                ret = 1; /* Malformed XML or could not open file */
                if (ops.listGood < 0)
                {
                    fprintf(stdout, "%s\n", argv[i]);
                }
            }
            if (ret) invalidFound = 1;

            if ((ops.listGood > 0) && (ret == 0))
                fprintf(stdout, "%s\n", argv[i]);
            if ((ops.listGood < 0) && (ret != 0))
                fprintf(stdout, "%s\n", argv[i]);
            /*
            if (ret == 0)
                fprintf(stderr, "%s - validates\n", argv[i]);
            else
                fprintf(stderr, "%s - invalid\n", argv[i]);
            */
        }

        xmlSchemaFreeValidCtxt(ctxt2);
        if (schema != NULL) xmlSchemaFree(schema);
        xmlSchemaCleanupTypes();
    }
#endif
    else if (ops.wellFormed)
    {
        int i;
        for (i=start; i<argc; i++)
        {
            xmlDocPtr doc;
            int ret;

            ret = 0;
            doc = NULL;

            if (!ops.err)
            {
                xmlDefaultSAXHandlerInit();
                xmlDefaultSAXHandler.error = NULL;
                xmlDefaultSAXHandler.warning = NULL;
            }

            doc = xmlParseFile(argv[i]);
            if (doc != NULL)
            {
                if (ops.listGood > 0)
                {
                    fprintf(stdout, "%s\n", argv[i]);
                }
                xmlFreeDoc(doc);
            }
            else
            {
                ret = 1; /* Malformed XML or could not open file */
                if (ops.listGood < 0)
                {
                    fprintf(stdout, "%s\n", argv[i]);
                }
            }
            if (ret) invalidFound = 1;
        }
    }
    
    xmlCleanupParser();
    return invalidFound;
}  

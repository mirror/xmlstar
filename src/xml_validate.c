/*  $Id: xml_validate.c,v 1.26 2003/09/18 01:56:54 mgrouch Exp $  */

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

#ifdef LIBXML_SCHEMAS_ENABLED
#include <libxml/relaxng.h>
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
    char *relaxng;            /* External Relax-NG Schema URL or file name */
    int   err;                /* Allow stderr messages */
    int   wellFormed;         /* Check if well formed only */
    int   listGood;           /* >0 list good, <0 list bad */
    int   show_val_res;       /* display file names and valid/invalid message */
} valOptions;

typedef valOptions *valOptionsPtr;

static const char validate_usage_str[] =
"XMLStarlet Toolkit: Validate XML document(s)\n"
"Usage: xml val <options> [ <xml-file-or-uri> ... ]\n"
"where <options>\n"
"   -w or --well-formed        - validate well-formedness only (default)\n"
"   -d or --dtd <dtd-file>     - validate against DTD\n"
#ifdef LIBXML_SCHEMAS_ENABLED
"   -s or --xsd <xsd-file>     - validate against XSD schema\n"
#endif
#ifdef LIBXML_SCHEMAS_ENABLED
"   -r or --relaxng <rng-file> - validate against Relax-NG schema\n"
#endif
/*"   -x or --xml-out            - print result as xml\n"*/
"   -e or --err                - print verbose error messages on stderr\n"
"   -b or --list-bad           - list only files which do not validate\n"
"   -g or --list-good          - list only files which validate\n"
"   -q or --quiet              - do not list files (return result code only)\n\n";

#ifdef LIBXML_SCHEMAS_ENABLED
static const char schema_notice[] =
"NOTE: XML Schemas are not fully supported yet due to its incomplete\n" 
"      support in libxml (see http://xmlsoft.org)\n\n";
#endif

/**
 *  display short help message
 */
void
valUsage(int argc, char **argv)
{
    extern const char more_info[];
    FILE* o = stderr;
    fprintf(o, validate_usage_str);
#ifdef LIBXML_SCHEMAS_ENABLED
    fprintf(o, schema_notice);
#endif
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
    ops->dtd = NULL;
    ops->schema = NULL;
    ops->relaxng = NULL;
    ops->show_val_res = 1;
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
        else if (!strcmp(argv[i], "--err") || !strcmp(argv[i], "-e"))
        {
            ops->err = 1;
            i++;
        }
        else if (!strcmp(argv[i], "--list-good") || !strcmp(argv[i], "-g"))
        {
            ops->listGood = 1;
            ops->show_val_res = 0;
            i++;
        }
        else if (!strcmp(argv[i], "--list-bad") || !strcmp(argv[i], "-b"))
        {
            ops->listGood = -1;
            ops->show_val_res = 0;
            i++;
        }
        else if (!strcmp(argv[i], "--quiet") || !strcmp(argv[i], "-q"))
        {
            ops->listGood = 0;
            ops->show_val_res = 0;
            i++;
        }
        else if (!strcmp(argv[i], "--dtd") || !strcmp(argv[i], "-d"))
        {
            i++;
            if (i >= argc) valUsage(argc, argv);
            ops->dtd = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "--xsd") || !strcmp(argv[i], "-s"))
        {
            i++;
            if (i >= argc) valUsage(argc, argv);
            ops->schema = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "--relaxng") || !strcmp(argv[i], "-r"))
        {
            i++;
            if (i >= argc) valUsage(argc, argv);
            ops->relaxng = argv[i];
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
                if ((ops->listGood < 0) && !ops->show_val_res)
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
                if ((ops->listGood > 0) && !ops->show_val_res)
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
 *  Do nothing function
 */
void
foo(void *ctx, const char *msg, ...)
{ 
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
    
    if (argc <= 2) valUsage(argc, argv);
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
                if ((ops.listGood < 0) && !ops.show_val_res)
                {
                    fprintf(stdout, "%s\n", argv[i]);
                }
            }
            if (ret) invalidFound = 1;     

            if (ops.show_val_res)
            {
                if (ret == 0)
                    fprintf(stdout, "%s - valid\n", argv[i]);
                else
                    fprintf(stdout, "%s - invalid\n", argv[i]);
            }
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
        if (!ops.err)
        {
            xmlSchemaSetParserErrors(ctxt,
                (xmlSchemaValidityErrorFunc) NULL,
                (xmlSchemaValidityWarningFunc) NULL,
                NULL);
            xmlGenericError = foo;
            xmlGenericErrorContext = NULL;
            xmlInitParser();
        }
        else
        {
            xmlSchemaSetParserErrors(ctxt,
                (xmlSchemaValidityErrorFunc) fprintf,
                (xmlSchemaValidityWarningFunc) fprintf,
                stderr);
        }
               
        schema = xmlSchemaParse(ctxt);
        xmlSchemaFreeParserCtxt(ctxt);
        if (schema != NULL)
        {                  
            for (i=start; i<argc; i++)
            {
                xmlDocPtr doc;
                int ret;

                ret = 0;
                doc = NULL;

                ctxt2 = xmlSchemaNewValidCtxt(schema);
                if (!ops.err)
                {
                    xmlSchemaSetValidErrors(ctxt2,
                        (xmlSchemaValidityErrorFunc) NULL,
                        (xmlSchemaValidityWarningFunc) NULL,
                        NULL);
                    xmlGenericError = foo;
                    xmlGenericErrorContext = NULL;
                    xmlInitParser();
                }
                else
                {
                    xmlSchemaSetValidErrors(ctxt2,
                        (xmlSchemaValidityErrorFunc) fprintf,
                        (xmlSchemaValidityWarningFunc) fprintf,
                        stderr);
                }

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
                }
                if (ret) invalidFound = 1;

                if (!ops.show_val_res)
                {
                    if ((ops.listGood > 0) && (ret == 0))
                        fprintf(stdout, "%s\n", argv[i]);
                    if ((ops.listGood < 0) && (ret != 0))
                        fprintf(stdout, "%s\n", argv[i]);
                }
                else
                {
                    if (ret == 0)
                        fprintf(stdout, "%s - valid\n", argv[i]);
                    else
                        fprintf(stdout, "%s - invalid\n", argv[i]);
                }
                
                if (ctxt2 != NULL) xmlSchemaFreeValidCtxt(ctxt2);
            }
        }
        else
        {
            invalidFound = 2;
        }
        if (schema != NULL) xmlSchemaFree(schema);
        xmlSchemaCleanupTypes();        
    }
#endif
#ifdef LIBXML_SCHEMAS_ENABLED
    else if (ops.relaxng)
    {
        xmlRelaxNGPtr relaxngschemas = NULL;
        xmlRelaxNGParserCtxtPtr ctxt = NULL;
        xmlRelaxNGValidCtxtPtr ctxt2 = NULL;
        int i;
        
        /* forces loading the DTDs */
        xmlLoadExtDtdDefaultValue |= 1;

        /* TODO: Do not print debug stuff */
        ctxt = xmlRelaxNGNewParserCtxt(ops.relaxng);
        if (!ops.err)
        {
            xmlRelaxNGSetParserErrors(ctxt,
                (xmlRelaxNGValidityErrorFunc) NULL,
                (xmlRelaxNGValidityWarningFunc) NULL,
                NULL);
            xmlGenericError = foo;
            xmlGenericErrorContext = NULL;
            xmlInitParser();
        }
        else
        {
            xmlRelaxNGSetParserErrors(ctxt,
                (xmlRelaxNGValidityErrorFunc) fprintf,
                (xmlRelaxNGValidityWarningFunc) fprintf,
                stderr);
        }

        relaxngschemas = xmlRelaxNGParse(ctxt);
        if (relaxngschemas == NULL) {
            xmlGenericError(xmlGenericErrorContext,
            "Relax-NG schema %s failed to compile\n", ops.relaxng);
            ops.relaxng = NULL;
        }
        xmlRelaxNGFreeParserCtxt(ctxt);     

        if (relaxngschemas != NULL)
        {
            for (i=start; i<argc; i++)
            {
                xmlDocPtr doc;
                int ret;

                ret = 0;
                doc = NULL;

                ctxt2 = xmlRelaxNGNewValidCtxt(relaxngschemas);
                if (!ops.err)
                {
                    xmlRelaxNGSetValidErrors(ctxt2,
                        (xmlRelaxNGValidityErrorFunc) NULL,
                        (xmlRelaxNGValidityWarningFunc) NULL,
                        NULL);
                    xmlGenericError = foo;
                    xmlGenericErrorContext = NULL;
                    xmlInitParser();
                }
                else
                {
                    xmlRelaxNGSetValidErrors(ctxt2,
                        (xmlRelaxNGValidityErrorFunc) fprintf,
                        (xmlRelaxNGValidityWarningFunc) fprintf,
                        stderr);
                }

                if (!ops.err)
                {
                    xmlDefaultSAXHandlerInit();
                    xmlDefaultSAXHandler.error = NULL;
                    xmlDefaultSAXHandler.warning = NULL;
                }

                doc = xmlParseFile(argv[i]);
                if (doc)
                {
                    ret = xmlRelaxNGValidateDoc(ctxt2, doc);
                    xmlFreeDoc(doc);
                }
                else
                {
                    ret = 1; /* Malformed XML or could not open file */
                }
                if (ret) invalidFound = 1;

                if (!ops.show_val_res)
                {
                    if ((ops.listGood > 0) && (ret == 0))
                        fprintf(stdout, "%s\n", argv[i]);
                    if ((ops.listGood < 0) && (ret != 0))
                        fprintf(stdout, "%s\n", argv[i]);
                }
                else
                {
                    if (ret == 0)
                        fprintf(stdout, "%s - valid\n", argv[i]);
                    else
                        fprintf(stdout, "%s - invalid\n", argv[i]);
                }

                if (ctxt2 != NULL) xmlRelaxNGFreeValidCtxt(ctxt2);
            }
        }
        else
        {
            invalidFound = 2;
        }
        if (relaxngschemas != NULL) xmlRelaxNGFree(relaxngschemas);
        xmlRelaxNGCleanupTypes();
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
                if ((ops.listGood > 0) && !ops.show_val_res)
                {
                    fprintf(stdout, "%s\n", argv[i]);
                }
                xmlFreeDoc(doc);
            }
            else
            {
                ret = 1; /* Malformed XML or could not open file */
                if ((ops.listGood < 0) && !ops.show_val_res)
                {
                    fprintf(stdout, "%s\n", argv[i]);
                }
            }
            if (ret) invalidFound = 1;

            if (ops.show_val_res)
            {
                if (ret == 0)
                    fprintf(stdout, "%s - valid\n", argv[i]);
                else
                    fprintf(stdout, "%s - invalid\n", argv[i]);
            }
        }
    }
    
    xmlCleanupParser();
    return invalidFound;
}


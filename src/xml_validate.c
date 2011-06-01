/*  $Id: xml_validate.c,v 1.36 2005/01/07 01:52:43 mgrouch Exp $  */

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "xmlstar.h"
#include "trans.h"

#ifdef LIBXML_SCHEMAS_ENABLED
#include <libxml/xmlschemas.h>
#include <libxml/xmlschemastypes.h>
#endif

#ifdef LIBXML_SCHEMAS_ENABLED
#include <libxml/relaxng.h>
#endif

#include <libxml/xmlreader.h>

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
    int   embed;              /* Validate using embeded DTD */
    int   wellFormed;         /* Check if well formed only */
    int   listGood;           /* >0 list good, <0 list bad */
    int   show_val_res;       /* display file names and valid/invalid message */
    int   nonet;              /* disallow network access */
} valOptions;

typedef valOptions *valOptionsPtr;

/*
 * usage string chunk : 509 char max on ISO C90
 */
static const char validate_usage_str_1[] =
"XMLStarlet Toolkit: Validate XML document(s)\n"
"Usage: %s val <options> [ <xml-file-or-uri> ... ]\n"
"where <options>\n"
"  -w or --well-formed        - validate well-formedness only (default)\n"
"  -d or --dtd <dtd-file>     - validate against DTD\n"
"  --net                      - allow network access\n";


static const char validate_usage_str_2[] =
#ifdef LIBXML_SCHEMAS_ENABLED
"  -s or --xsd <xsd-file>     - validate against XSD schema\n"
"  -E or --embed              - validate using embedded DTD\n"
#endif
#ifdef LIBXML_SCHEMAS_ENABLED
"  -r or --relaxng <rng-file> - validate against Relax-NG schema\n"
#endif
"  -e or --err                - print verbose error messages on stderr\n"
"  -b or --list-bad           - list only files which do not validate\n"
"  -g or --list-good          - list only files which validate\n"
"  -q or --quiet              - do not list files (return result code only)\n\n";

#ifdef LIBXML_SCHEMAS_ENABLED
static const char schema_notice[] =
"NOTE: XML Schemas are not fully supported yet due to its incomplete\n" 
"      support in libxml2 (see http://xmlsoft.org)\n\n";
#endif

/**
 *  display short help message
 */
void
valUsage(int argc, char **argv, exit_status status)
{
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprintf(o, validate_usage_str_1, argv[0]);
    fprintf(o, "%s", validate_usage_str_2);
#ifdef LIBXML_SCHEMAS_ENABLED
    fprintf(o, "%s", schema_notice);
#endif
    fprintf(o, "%s", more_info);
    exit(status);
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
    ops->embed = 0;
    ops->dtd = NULL;
    ops->schema = NULL;
    ops->relaxng = NULL;
    ops->show_val_res = 1;
    ops->nonet = 1;
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
        else if (!strcmp(argv[i], "--embed") || !strcmp(argv[i], "-E"))
        {
            ops->embed = 1;
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
            if (i >= argc) valUsage(argc, argv, EXIT_BAD_ARGS);
            ops->dtd = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "--xsd") || !strcmp(argv[i], "-s"))
        {
            i++;
            if (i >= argc) valUsage(argc, argv, EXIT_BAD_ARGS);
            ops->schema = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "--relaxng") || !strcmp(argv[i], "-r"))
        {
            i++;
            if (i >= argc) valUsage(argc, argv, EXIT_BAD_ARGS);
            ops->relaxng = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "--net"))
        {
            ops->nonet = 0;
            i++;
        }
        else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h"))
        {
            valUsage(argc, argv, EXIT_SUCCESS);
        }
        else if (!strcmp(argv[i], "-"))
        {
            i++;
            break;
        }
        else if (argv[i][0] == '-')
        {
            valUsage(argc, argv, EXIT_BAD_ARGS);
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

#if !defined(LIBXML_VALID_ENABLED)
	xmlGenericError(xmlGenericErrorContext,
	"libxml2 has no validation support");
	return 2;
#endif
        dtd = xmlParseDTD(NULL, (const xmlChar *)dtdvalid);
        if (dtd == NULL)
        {
            xmlGenericError(xmlGenericErrorContext,
            "Could not parse DTD %s\n", dtdvalid);
            result = 2;
        }
        else
        {
            xmlValidCtxtPtr cvp;

            if ((cvp = xmlNewValidCtxt()) == NULL) 
            {
                xmlGenericError(xmlGenericErrorContext,
                    "Couldn't allocate validation context\n");
                exit(-1);
            }
        
            if (ops->err)
            {
                cvp->userData = (void *) stderr;
                cvp->error    = (xmlValidityErrorFunc) fprintf;
                cvp->warning  = (xmlValidityWarningFunc) fprintf;
            }
            else
            {
                cvp->userData = (void *) NULL;
                cvp->error    = (xmlValidityErrorFunc) NULL;
                cvp->warning  = (xmlValidityWarningFunc) NULL;
            }
                        
            if (!xmlValidateDtd(cvp, doc, dtd))
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
            xmlFreeValidCtxt(cvp);
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
    static ErrorInfo errorInfo;
    int invalidFound = 0;
    int options = XML_PARSE_DTDLOAD | XML_PARSE_DTDATTR;

    if (argc <= 2) valUsage(argc, argv, EXIT_BAD_ARGS);
    valInitOptions(&ops);
    start = valParseOptions(&ops, argc, argv);
    if (ops.nonet) options |= XML_PARSE_NONET;

    errorInfo.verbose = ops.err;
    xmlSetStructuredErrorFunc(&errorInfo, reportError);
    xmlLineNumbersDefault(1);

    if (ops.dtd)
    {
        /* xmlReader doesn't work with external dtd, have to use SAX
         * interface */
        int i;

        for (i=start; i<argc; i++)
        {
            xmlDocPtr doc;
            int ret;

            ret = 0;
            doc = NULL;

            errorInfo.filename = argv[i];
            doc = xmlReadFile(argv[i], NULL, options);
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
    else if (ops.schema || ops.relaxng || ops.embed || ops.wellFormed)
    {
        int i;
        xmlTextReaderPtr reader = NULL;

#ifdef LIBXML_SCHEMAS_ENABLED
        xmlSchemaPtr schema = NULL;
        xmlSchemaParserCtxtPtr schemaParserCtxt = NULL;
        xmlSchemaValidCtxtPtr schemaCtxt = NULL;

        xmlRelaxNGPtr relaxng = NULL;
        xmlRelaxNGParserCtxtPtr relaxngParserCtxt = NULL;
        /* there is no xmlTextReaderRelaxNGValidateCtxt() !?  */

        /* TODO: Do not print debug stuff */
        if (ops.schema)
        {
            schemaParserCtxt = xmlSchemaNewParserCtxt(ops.schema);
            if (!schemaParserCtxt)
            {
                invalidFound = 2;
                goto schemaCleanup;
            }
            errorInfo.filename = ops.schema;
            schema = xmlSchemaParse(schemaParserCtxt);
            if (!schema)
            {
                invalidFound = 2;
                goto schemaCleanup;
            }

            xmlSchemaFreeParserCtxt(schemaParserCtxt);
            schemaCtxt = xmlSchemaNewValidCtxt(schema);
            if (!schemaCtxt)
            {
                invalidFound = 2;
                goto schemaCleanup;
            }

        }
        else if (ops.relaxng)
        {
            relaxngParserCtxt = xmlRelaxNGNewParserCtxt(ops.relaxng);
            if (!relaxngParserCtxt)
            {
                invalidFound = 2;
                goto schemaCleanup;
            }

            errorInfo.filename = ops.relaxng;
            relaxng = xmlRelaxNGParse(relaxngParserCtxt);
            if (!relaxng)
            {
                invalidFound = 2;
                goto schemaCleanup;
            }

        }
#endif  /* LIBXML_SCHEMAS_ENABLED */

        for (i=start; i<argc; i++)
        {
            int ret = 0;
            if (ops.embed) options |= XML_PARSE_DTDVALID;

            if (!reader)
            {
                reader = xmlReaderForFile(argv[i], NULL, options);
            }
            else
            {
                ret = xmlReaderNewFile(reader, argv[i], NULL, options);
            }

            errorInfo.xmlReader = reader;
            errorInfo.filename = argv[i];

            if (reader && ret == 0)
            {
#ifdef LIBXML_SCHEMAS_ENABLED
                if (schemaCtxt)
                {
                    ret = xmlTextReaderSchemaValidateCtxt(reader,
                        schemaCtxt, 0);
                }
                else if (relaxng)
                {
                    ret = xmlTextReaderRelaxNGSetSchema(reader,
                        relaxng);
                }
#endif  /* LIBXML_SCHEMAS_ENABLED */

                if (ret == 0)
                {
                    do
                    {
                        ret = xmlTextReaderRead(reader);
                    } while (ret == 1);
                    if (ret != -1 && (schema || relaxng || ops.embed))
                        ret = !xmlTextReaderIsValid(reader);
                }
            }
            else
            {
                if (ops.err)
                    fprintf(stderr, "couldn't read file '%s'\n", errorInfo.filename);
                ret = 1; /* could not open file */
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
        }
        errorInfo.xmlReader = NULL;
        xmlFreeTextReader(reader);

#ifdef LIBXML_SCHEMAS_ENABLED
    schemaCleanup:
        xmlSchemaFreeValidCtxt(schemaCtxt);
        xmlRelaxNGFree(relaxng);
        xmlSchemaFree(schema);
        xmlRelaxNGCleanupTypes();
        xmlSchemaCleanupTypes();
#endif  /* LIBXML_SCHEMAS_ENABLED */
    }

    xmlCleanupParser();
    return invalidFound;
}

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
    const char *dtd;          /* External DTD URL or file name */
    const char *schema;       /* External Schema URL or file name */
    const char *relaxng;      /* External Relax-NG Schema URL or file name */
    int   err;                /* Allow stderr messages */
    int   embed;              /* Validate using embeded DTD */
    int   wellFormed;         /* Check if well formed only */
    int   listGood;           /* >0 list good, <0 list bad */
    int   show_val_res;       /* display file names and valid/invalid message */
    int   nonet;              /* disallow network access */
} valOptions;

typedef valOptions *valOptionsPtr;

/**
 *  display short help message
 */
void
valUsage(exit_status status)
{
    extern void fprint_validate_usage(FILE* o, const char* argv0);
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprint_validate_usage(o, get_arg(ARG0));
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
void
valParseOptions(valOptionsPtr ops)
{
    for (;;)
    {
        const char* arg = get_arg(OPTION_NEXT);
        if (!arg) break;

        if (strcmp(arg, "--well-formed") == 0 || strcmp(arg, "-w") == 0) {
            ops->wellFormed = 1;
        } else if (strcmp(arg, "--err") == 0 || strcmp(arg, "-e") == 0) {
            ops->err = 1;
        } else if (strcmp(arg, "--embed") == 0 || strcmp(arg, "-E") == 0) {
            ops->embed = 1;
        } else if (strcmp(arg, "--list-good") == 0 || strcmp(arg, "-g") == 0) {
            ops->listGood = 1;
            ops->show_val_res = 0;
        } else if (strcmp(arg, "--list-bad") == 0 || strcmp(arg, "-b") == 0) {
            ops->listGood = -1;
            ops->show_val_res = 0;
        } else if (strcmp(arg, "--quiet") == 0 || strcmp(arg, "-q") == 0) {
            ops->listGood = 0;
            ops->show_val_res = 0;
        } else if (strcmp(arg, "--dtd") == 0 || strcmp(arg, "-d") == 0) {
            arg = get_arg(ARG_NEXT);
            if (!arg) {
                fprintf(stderr, "expected <dtd-file>\n");
                exit(EXIT_BAD_ARGS);
            }
            ops->dtd = arg;
        } else if (strcmp(arg, "--xsd") == 0 || strcmp(arg, "-s") == 0) {
            arg = get_arg(ARG_NEXT);
            if (!arg) {
                fprintf(stderr, "expected <xsd-file>\n");
                exit(EXIT_BAD_ARGS);
            }
            ops->schema = arg;
        } else if (strcmp(arg, "--relaxng") == 0 || strcmp(arg, "-r") == 0) {
            arg = get_arg(ARG_NEXT);
            if (!arg) {
                fprintf(stderr, "expected <rng-file>\n");
                exit(EXIT_BAD_ARGS);
            }
            ops->relaxng = arg;
        } else if (strcmp(arg, "--net") == 0) {
            ops->nonet = 0;
        } else if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0) {
            valUsage(EXIT_SUCCESS);
        } else {
            fprintf(stderr, "unrecognized option %s\n", arg);
        }
    }
}

/**
 *  Validate XML document against DTD
 */
int
valAgainstDtd(valOptionsPtr ops, const char* dtdvalid, xmlDocPtr doc, const char* filename)
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
valMain(void)
{
    static valOptions ops;
    static ErrorInfo errorInfo;
    int invalidFound = 0;
    int options = XML_PARSE_DTDLOAD | XML_PARSE_DTDATTR;

    if (!get_arg(ARG_PEEK)) valUsage(EXIT_BAD_ARGS);
    valInitOptions(&ops);
    valParseOptions(&ops);
    if (ops.nonet) options |= XML_PARSE_NONET;

    errorInfo.verbose = ops.err;
    xmlSetStructuredErrorFunc(&errorInfo, reportError);
    xmlLineNumbersDefault(1);

    if (ops.dtd)
    {
        /* xmlReader doesn't work with external dtd, have to use SAX
         * interface */
        for (;;) {
            xmlDocPtr doc = NULL;
            int ret = 0;
            const char* filename = get_arg(ARG_NEXT);
            if (!filename) break;

            errorInfo.filename = filename;
            doc = xmlReadFile(filename, NULL, options);
            if (doc) {
                /* TODO: precompile DTD once */                
                ret = valAgainstDtd(&ops, ops.dtd, doc, filename);
                xmlFreeDoc(doc);
            } else {
                ret = 1; /* Malformed XML or could not open file */
                if ((ops.listGood < 0) && !ops.show_val_res)
                    fprintf(stdout, "%s\n", filename);
            }
            if (ret) invalidFound = 1;     

            if (ops.show_val_res) {
                fprintf(stdout, "%s - %s\n",
                    filename, (ret == 0)? "valid" : "invalid");
            }
        }
    } else if (ops.schema || ops.relaxng || ops.embed || ops.wellFormed) {
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

        for (;;) {
            int ret = 0;
            const char* filename = get_arg(ARG_NEXT);
            if (!filename) break;

            if (ops.embed) options |= XML_PARSE_DTDVALID;

            if (!reader)
                reader = xmlReaderForFile(filename, NULL, options);
            else
                ret = xmlReaderNewFile(reader, filename, NULL, options);

            errorInfo.xmlReader = reader;
            errorInfo.filename = filename;

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
                    fprintf(stdout, "%s\n", filename);
                if ((ops.listGood < 0) && (ret != 0))
                    fprintf(stdout, "%s\n", filename);
            }
            else
            {
                if (ret == 0)
                    fprintf(stdout, "%s - valid\n", filename);
                else
                    fprintf(stdout, "%s - invalid\n", filename);
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

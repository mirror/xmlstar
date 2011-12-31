/* $Id: xml_pyx.c,v 1.9 2005/03/12 03:24:23 mgrouch Exp $ */

/**
 *  Based on xmln from pyxie project
 *
 *  The PYX format is a line-oriented representation of
 *  XML documents that is derived from the SGML ESIS format.
 *  (see ESIS - ISO 8879 Element Structure Information Set spec,
 *  ISO/IEC JTC1/SC18/WG8 N931 (ESIS))
 *
 *  A non-validating, ESIS generating tool
 *  ESIS Generation by Sean Mc Grath http://www.digitome.com/sean.html
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h>

#include "xmlstar.h"

/*
 * usage string chunk : 509 char max on ISO C90
 */
static const char pyx_usage_str_1[] =
"XMLStarlet Toolkit: Convert XML into PYX format (based on ESIS - ISO 8879)\n"
"Usage: %s pyx {<xml-file>}\n"
"where\n"
"  <xml-file> - input XML document file name (stdin is used if missing)\n\n";

static const char pyx_usage_str_2[] =
"The PYX format is a line-oriented representation of\n"
"XML documents that is derived from the SGML ESIS format.\n"
"(see ESIS - ISO 8879 Element Structure Information Set spec,\n"
"ISO/IEC JTC1/SC18/WG8 N931 (ESIS))\n\n";

static const char pyx_usage_str_3[] =
"A non-validating, ESIS generating tool originally developed for\n"
"pyxie project (see http://pyxie.sourceforge.net/)\n"
"ESIS Generation by Sean Mc Grath http://www.digitome.com/sean.html\n\n";

/**
 *  Output newline and tab characters as escapes
 *  Required both for attribute values and character data (#PCDATA)
 */
static void
SanitizeData(const xmlChar *s, int len)
{
    while (len--)
    {
        switch (*s)
        {
            case 10:
                printf("\\n");
                break;
            case 13:
                break;
            case 9:
                printf ("\\t");
                break;
            case '\\':
                printf ("\\\\");
                break;
            default:
                putchar (*s);
        }
        s++;
    }
}

static void
print_qname(const xmlChar *prefix, const xmlChar *localname)
{
    if (prefix)
        printf("%s:", prefix);
    printf("%s", localname);
}

int
CompareAttributes(const void *a1,const void *a2)
{
    typedef xmlChar const *const xmlCStr;
    xmlCStr *attr1 = a1, *attr2 = a2;
    return xmlStrcmp(*attr1, *attr2);
}

void
pyxStartElement (void * ctx,
    const xmlChar * localname,
    const xmlChar * prefix,
    const xmlChar * URI,
    int nb_namespaces,
    const xmlChar ** namespaces,
    int nb_attributes,
    int nb_defaulted,
    const xmlChar ** attributes)
{
    int i;
    fprintf(stdout,"(");
    print_qname(prefix, localname);
    fprintf(stdout, "\n");


    if (nb_attributes > 1)
        /* Sort the pairs based on the name part of the pair */
        qsort ((void *)attributes,
            nb_attributes,
            sizeof(xmlChar *)*5,
            CompareAttributes);

    for (i = 0; i < nb_attributes; i++) {
        int aidx = i * 5;
        const xmlChar *localname = attributes[aidx],
            *prefix = attributes[aidx+1],
            /* *nsURI = attributes[aidx+2], */
            *valueBegin = attributes[aidx+3],
            *valueEnd = attributes[aidx+4];
        int valueLen = valueEnd - valueBegin;

        /* Attribute Name */
        putchar('A');
        print_qname(prefix, localname);
        putchar(' ');
        /* value - can contain literal "\n" so escape */
        SanitizeData(valueBegin, valueLen);
        putchar('\n');
    }
}

void
pyxEndElement(void *userData, const xmlChar *localname, const xmlChar *prefix,
    const xmlChar *URI)
{
    fprintf(stdout,")");
    print_qname(prefix, localname);
    putchar('\n');
}

void
pyxCharacterData(void *userData, const xmlChar *s, int len)
{
    fprintf(stdout, "-");
    SanitizeData(s, len);
    putchar('\n');
}

void
pyxProcessingInstruction(void *userData, 
                         const xmlChar *target, 
                         const xmlChar *data)
{
    fprintf(stdout,"?%s ",target);
    SanitizeData(data, xmlStrlen(data));
    fprintf(stdout,"\n");
}

void
pyxUnparsedEntityDeclHandler(void *userData,
                             const xmlChar *entityName,
                             const xmlChar *publicId,
                             const xmlChar *systemId,
                             const xmlChar *notationName)
{
    fprintf(stdout, "U%s %s %s%s%s\n", 
           (char *)entityName, (char *)notationName, (char *)systemId,
           (publicId == NULL? "": " "), 
           (publicId == NULL? "": (char *) publicId));
}

void
pyxNotationDeclHandler(void *userData,
                       const xmlChar *notationName,
                       const xmlChar *publicId,
                       const xmlChar *systemId)
{
    fprintf(stdout, "N%s %s%s%s\n", (char*) notationName, (char*) systemId,
           (publicId == NULL? "": " "), 
           (publicId == NULL? "": (const char*) publicId));
}

void
pyxExternalEntityReferenceHandler(void* userData,
                                  const xmlChar *name)
{
    const xmlChar *p = name;
    fprintf (stdout, "&");
    /* Up to space is the name of the referenced entity */
    while (*p && (*p != ' ')) {
        putchar (*p);
        p++;
    }
}

static void
pyxExternalSubsetHandler(void *ctx ATTRIBUTE_UNUSED, const xmlChar *name,
                         const xmlChar *ExternalID, const xmlChar *SystemID)
{
    fprintf(stdout, "D %s PUBLIC", name); /* TODO: re-check */
    if (ExternalID == NULL)
        fprintf(stdout, " ");
    else
        fprintf(stdout, " \"%s\"", ExternalID);
    if (SystemID == NULL)
        fprintf(stdout, "\n");
    else
        fprintf(stdout, " \"%s\"\n", SystemID);
}

static void
pyxCommentHandler(void *ctx ATTRIBUTE_UNUSED, const xmlChar *value)
{
    fprintf(stdout,"C");
    SanitizeData(value, xmlStrlen(value));
    fprintf(stdout,"\n");
}

static void
pyxCdataBlockHandler(void *ctx ATTRIBUTE_UNUSED, const xmlChar *value, int len)
{
    fprintf(stdout,"[");
    SanitizeData(value, len);
    fprintf(stdout,"\n");
}

static void
pyxUsage(const char *argv0, exit_status status)
{
    extern const char more_info[];
    FILE *o = (status == EXIT_SUCCESS)? stdout : stderr;
    fprintf(o, pyx_usage_str_1, argv0);
    fprintf(o, "%s", pyx_usage_str_2);
    fprintf(o, "%s", pyx_usage_str_3);
    fprintf(o, "%s", more_info);
    exit(status);
}

int
pyx_process_file(const char *filename)
{
    int ret;
    xmlParserCtxtPtr ctxt;

    xmlInitParser();
    ctxt = xmlCreateFileParserCtxt(filename);

    memset(ctxt->sax, 0, sizeof(*ctxt->sax));

    /* Establish Event Handlers */
    ctxt->sax->initialized = XML_SAX2_MAGIC;
    ctxt->sax->startElementNs = pyxStartElement;
    ctxt->sax->endElementNs = pyxEndElement;
    ctxt->sax->processingInstruction = pyxProcessingInstruction;
    ctxt->sax->characters = pyxCharacterData;
    ctxt->sax->notationDecl = pyxNotationDeclHandler;
    ctxt->sax->reference = pyxExternalEntityReferenceHandler;
    ctxt->sax->unparsedEntityDecl = pyxUnparsedEntityDeclHandler;
    ctxt->sax->externalSubset = pyxExternalSubsetHandler;
    ctxt->sax->comment = pyxCommentHandler;
    ctxt->sax->cdataBlock = pyxCdataBlockHandler;

    ret = xmlParseDocument(ctxt);
    xmlFreeParserCtxt(ctxt);
    xmlCleanupParser();

    return ret;
}

int
pyxMain(int argc,const char *argv[])
{
    int status = 0;

    if ((argc > 2) &&
        (
           (strcmp(argv[2],"-h") == 0) ||
           (strcmp(argv[2],"-H") == 0) ||
           (strcmp(argv[2],"-Z") == 0) ||
           (strcmp(argv[2],"-?") == 0) ||
           (strcmp(argv[2],"--help") == 0)
       ))
    {
        pyxUsage(argv[0], EXIT_SUCCESS);
    }
    if (argc == 2) {
        status = pyx_process_file("-");
    }
    else {
        argv++;
        argc--;
        for (++argv; argc>1; argc--,argv++) {
            int ret = pyx_process_file(*argv);
            if (ret != 0) status = ret;
        }
    }
    return status;
}

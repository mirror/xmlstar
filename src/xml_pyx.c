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
SanitizeData(const char *s, int len)
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

int
CompareAttributes(const void *a1,const void *a2)
{
    return xmlStrcmp(*(unsigned char* const *)a1, *(unsigned char* const *)a2);
}

void
pyxStartElement(void *userData, const xmlChar *name, const xmlChar **atts)
{
    const xmlChar **p;
    int AttributeCount;
    fprintf (stdout,"(%s\n",name);

    if (atts != NULL)
    {
        /* Count the number of attributes */
        for (p = atts; *p != NULL; p++);

        AttributeCount = (p - atts) >> 1; /* (name,value) pairs 
                                           so divide by two */
        if (AttributeCount > 1)
            /* Sort the pairs based on the name part of the pair */
            qsort ((void *)atts,
                    AttributeCount,
                    sizeof(char *)*2,
                    CompareAttributes);

        while (*atts) {
            /* Attribute Name */
            fprintf (stdout,"A%s ",*atts);
            atts++; /* Now pointing at value - can contain literal "\n" 
                       so escape */
            SanitizeData((const char *) *atts, xmlStrlen(*atts));
            atts++;
            putchar('\n');
        }
    }
}

void
pyxEndElement(void *userData, const xmlChar *name)
{
    fprintf(stdout,")%s\n",name);
}

void
pyxCharacterData(void *userData, const xmlChar *s, int len)
{
    fprintf(stdout, "-");
    SanitizeData((const char *) s,len);
    putchar('\n');
}

void
pyxProcessingInstruction(void *userData, 
                         const xmlChar *target, 
                         const xmlChar *data)
{
    fprintf(stdout,"?%s ",target);
    SanitizeData((const char *) data, xmlStrlen(data));
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
    SanitizeData((const char *) value, xmlStrlen(value));
    fprintf(stdout,"\n");
}

static void
pyxCdataBlockHandler(void *ctx ATTRIBUTE_UNUSED, const xmlChar *value, int len)
{
    fprintf(stdout,"[");
    SanitizeData((const char *) value, len);
    fprintf(stdout,"\n");
}

static void
pyxUsage(const char *argv0, exit_status status)
{
    extern const char more_info[];
    FILE* o = stderr;
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

    /* Establish Event Handlers */
    static xmlSAXHandler xmlSAX_handler;

    xmlInitParser();

    memset(&xmlSAX_handler, 0, sizeof(xmlSAX_handler));

    xmlSAX_handler.startElement = pyxStartElement;
    xmlSAX_handler.endElement = pyxEndElement;
    xmlSAX_handler.processingInstruction = pyxProcessingInstruction;
    xmlSAX_handler.characters = pyxCharacterData;
    xmlSAX_handler.notationDecl = pyxNotationDeclHandler;
    xmlSAX_handler.reference = pyxExternalEntityReferenceHandler;
    xmlSAX_handler.unparsedEntityDecl = pyxUnparsedEntityDeclHandler;
    xmlSAX_handler.externalSubset = pyxExternalSubsetHandler;
    xmlSAX_handler.comment = pyxCommentHandler;
    xmlSAX_handler.cdataBlock = pyxCdataBlockHandler;

    ret = xmlSAXUserParseFile(&xmlSAX_handler, NULL, filename);
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

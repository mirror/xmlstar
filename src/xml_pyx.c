/* $Id: xml_pyx.c,v 1.2 2003/06/12 01:36:54 mgrouch Exp $ */

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

static const char pyx_usage_str[] =
"XMLStarlet Toolkit: Convert XML into PYX format (based on ESIS - ISO 8879)\n"
"Usage: xml pyx {<xml-file>}\n"
"where\n"
"   <xml-file> - input XML document file name (stdin is used if missing)\n\n"
"The PYX format is a line-oriented representation of\n"
"XML documents that is derived from the SGML ESIS format.\n"
"(see ESIS - ISO 8879 Element Structure Information Set spec,\n"
"ISO/IEC JTC1/SC18/WG8 N931 (ESIS))\n"
"\n"
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
    return strcmp (*(const char **)a1,*(const char **)a2);
}

void
pyxStartElement(void *userData, const char *name, const char **atts)
{
    const char **p;
    int AttributeCount;
    fprintf (stdout,"(%s\n",name);

    if (atts != NULL)
    {
        /* Count the number of attributes */
        for (p = atts; *p != NULL; p++);

        AttributeCount = (p - atts) >> 1; /* (name,value) pairs so divide by two */
        if (AttributeCount > 1)
            /* Sort the pairs based on the name part of the pair */
            qsort ((void *)atts,AttributeCount,sizeof(char *)*2,CompareAttributes);

        while (*atts) {
            /* Attribute Name */
            fprintf (stdout,"A%s ",*atts);
            atts++; /* Now pointing at value - can contain literal "\n" so escape */
            SanitizeData(*atts,strlen(*atts));
            atts++;
            putchar('\n');
        }
    }
}

void
pyxEndElement(void *userData, const char *name)
{
    fprintf (stdout,")%s\n",name);
}

void
pyxCharacterData(void *userdata, const char *s, int len)
{
    fprintf (stdout, "-");
    SanitizeData (s,len);
    putchar ('\n');
}

void
pyxProcessingInstruction(void *userdata, const xmlChar *target, const xmlChar *data)
{
    fprintf(stdout,"?%s ",target);
    SanitizeData((const char *) data, strlen((const char *) data));
    fprintf(stdout,"\n");
}

void
pyxUnparsedEntityDeclHandler(void *userdata,
                             const char *entityName,
                             const char *base,
                             const char *systemId,
                             const char *publicId,
                             const char *notationName)
{
    fprintf(stdout, "U%s %s %s%s%s\n", entityName, notationName, systemId,
           (publicId == NULL? "": " "), (publicId == NULL? "": publicId));
}

void
pyxNotationDeclHandler(void *userData,
                       const xmlChar *notationName,
                       const xmlChar *base,
                       const xmlChar *systemId,
                       const xmlChar *publicId)
{
    fprintf(stdout, "N%s %s%s%s\n", (char*) notationName, (char*) systemId,
           (publicId == NULL? "": " "), (publicId == NULL? "": (const char*) publicId));
}

int
pyxExternalEntityReferenceHandler(void* parser,
                                  const xmlChar *openEntityNames,
                                  const xmlChar *base,
                                  const xmlChar *systemId,
                                  const xmlChar *publicId)
{
    const char *p = (const char *) openEntityNames;
    fprintf (stdout, "&");
    /* Up to space is the name of the referenced entity */
    while (*p && (*p != ' ')) {
        putchar (*p);
        p++;
    }
    fprintf(stdout, " %s%s%s\n", systemId,
           (publicId == NULL ? "": " "), (publicId == NULL ? "": (const char *)publicId));
    /* Indicate success */
    return 1;
}

static void
pyxUsage()
{
    extern const char more_info[];
    FILE* o = stderr;
    fprintf(o, pyx_usage_str);
    fprintf(o, more_info);
    exit(1);
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

    ret = xmlSAXUserParseFile(&xmlSAX_handler, NULL, filename);
    xmlCleanupParser();

    return ret;
}

int
pyxMain(int argc,const char *argv[])
{
    if ((argc > 2) &&
        (
           (strcmp(argv[2],"-h") == 0) ||
           (strcmp(argv[2],"-H") == 0) ||
           (strcmp(argv[2],"-Z") == 0) ||
           (strcmp(argv[2],"-?") == 0) ||
           (strcmp(argv[2],"--help") == 0)
       ))
    {
        pyxUsage();
        exit(0);
    }
    if (argc == 2) {
        pyx_process_file("-");
    }
    else {
        argv++;
        argc--;
        for (++argv; argc>1; argc--,argv++) {
            pyx_process_file(*argv);
        }
    }
    return 0;
}

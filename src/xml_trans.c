/*  $Id: xml_trans.c,v 1.15 2002/11/26 04:02:46 mgrouch Exp $  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "trans.h"

/*
 *  TODO:
 *        1. proper command line arguments handling
 *        2. review and clean up all code
 *        3. tests
 *        4. --novalid option analog (no dtd validation)
 *        5. --nonet option analog (no network for external entities)
 *        6. html and docbook input documents
 *        7. -s for string parameters instead of -p
 *        8. check embedded stylesheet support
 */

static const char trans_usage_str[] =
"XMLStarlet Toolkit: Transform XML document(s) using XSLT\n"
"Usage: xml tr [<options>] <xsl-file> {-p|-s <name>=<value>} [ <xml-file> ... ]\n"
"where\n"
"   <xsl-file>      - main XSLT stylesheet for transformation\n"
"   <xml-file>      - input XML document file name (stdin is used if missing)\n"
"   <name>=<value>  - name and value of the parameter passed to XSLT processor\n"
"   -p              - parameter is an XPATH expression (\"'string'\" to quote string)\n"
"   -s              - parameter is a string literal\n"
"<options> are:\n"
"   --show-ext      - show list of extensions\n"
"   --noval         - do not validate against DTDs or schemas\n"
"   --nonet         - refuse to fetch DTDs or entities over network\n"
#ifdef LIBXML_XINCLUDE_ENABLED
"   --xinclude      - do XInclude processing on document input\n"
#endif
"   --maxdepth val  - increase the maximum depth\n"
#ifdef LIBXML_HTML_ENABLED
"   --html          - input document(s) is(are) in HTML format\n"
#endif
#ifdef LIBXML_DOCB_ENABLED
"   --docbook       - input document(s) is(are) in SGML docbook format\n"
#endif
#ifdef LIBXML_CATALOG_ENABLED
"   --catalogs      - use SGML catalogs from $SGML_CATALOG_FILES\n"
"                     otherwise XML Catalogs starting from\n"
"                     file:///etc/xml/catalog are activated by default\n\n";
#endif

/**
 *  Display usage syntax
 */
void
trUsage(int argc, char **argv)
{
    extern const char more_info[];
    extern const char libxslt_more_info[];
    FILE* o = stderr;
    fprintf(o, trans_usage_str);
    fprintf(o, more_info);
    fprintf(o, libxslt_more_info);
    exit(1);
}

/**
 *  Parse global command line options
 */
void
trParseOptions(xsltOptionsPtr ops, int argc, char **argv)
{
    int i;
    
    if (argc <= 2) return;
    for (i=2; i<argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "--show-ext"))
            {
                ops->show_extensions = 1;
            }
            else if (!strcmp(argv[i], "--noval"))
            {
                ops->noval = 1;
            }
            else if (!strcmp(argv[i], "--nonet"))
            {
                ops->nonet = 1;
            }
#ifdef LIBXML_XINCLUDE_ENABLED
            else if (!strcmp(argv[i], "--xinclude"))
            {
                ops->xinclude = 1;
            }
#endif
#ifdef LIBXML_HTML_ENABLED
            else if (!strcmp(argv[i], "--html"))
            {
                ops->html = 1;
            }
#endif
#ifdef LIBXML_DOCB_ENABLED
            else if (!strcmp(argv[i], "--docbook"))
            {
                ops->docbook = 1;
            }
#endif
            else if (!strcmp(argv[i], "--nonet"))
            {
                ops->nonet = 1;
            }
        }
        else
            break;
    }
}

/**
 *  Initialize LibXML
 */
void
trInitLibXml(xsltOptionsPtr ops)
{
    /*
     * Initialize library memory
     */
    xmlInitMemory();

    LIBXML_TEST_VERSION

    /*
     * Store line numbers in the document tree
     */
    xmlLineNumbersDefault(1);

    /*
     * Register the EXSLT extensions
     */
    exsltRegisterAll();

    /*
     * Register the test module
    */
    xsltRegisterTestModule();

    if (ops->show_extensions) xsltDebugDumpExtensions(stderr);

    /*
     * Register entity loader
     */
    defaultEntityLoader = xmlGetExternalEntityLoader();
    xmlSetExternalEntityLoader(xsltExternalEntityLoader);
    if (ops->nonet) defaultEntityLoader = xmlNoNetExternalEntityLoader;
    
    if (ops->noval == 0)
        xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;
    else
        xmlLoadExtDtdDefaultValue = 0;

#ifdef LIBXML_XINCLUDE_ENABLED
    if (ops->xinclude)
        xsltSetXIncludeDefault(1);
#endif

#ifdef LIBXML_CATALOG_ENABLED
    if (ops->catalogs)
    {
        char *catalogs = getenv("SGML_CATALOG_FILES");
        if (catalogs == NULL)
            fprintf(stderr, "Variable $SGML_CATALOG_FILES not set\n");
        else
            xmlLoadCatalogs(catalogs);
    }    
#endif

    /*
     * Replace entities with their content.
     */
    xmlSubstituteEntitiesDefault(1);
}

/**
 *  Cleanup memory
 */
void
trCleanup()
{
    xsltCleanupGlobals();
    xmlCleanupParser();
#if 0
    xmlMemoryDump();
#endif
}

/**
 *  This is the main function for 'tr' option
 */
int
trMain(int argc, char **argv)
{
    xsltOptions ops;

    int errorno = 0;
    int i;
    
    if (argc <= 2) trUsage(argc, argv);

    xsltInitOptions(&ops);
    trParseOptions(&ops, argc, argv);
    trInitLibXml(&ops);

    /* find xsl file name */
    for (i=2; i<argc; i++)
        if (argv[i][0] != '-') break;

    /* set parameters */
    /* TODO */
/*
printf ("%s\n", argv[i]);
printf ("ops->html %d\n", ops.html);
*/
        
    /* run transformation */
    errorno = xsltRun(&ops, argv[i], argc-i-1, argv+i+1);

    trCleanup();
    
    return(errorno);                                                
}

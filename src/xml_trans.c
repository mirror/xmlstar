/*  $Id: xml_trans.c,v 1.12 2002/11/23 23:45:06 mgrouch Exp $  */

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "trans.h"

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
"   --xinclude      - do XInclude processing on document input\n"
"   --maxdepth val  - increase the maximum depth\n"
"   --html          - input document(s) is(are) in HTML format\n"
"   --docbook       - input document(s) is(are) in SGML docbook format\n"
"   --catalogs      - use SGML catalogs from $SGML_CATALOG_FILES\n"
"                     otherwise XML Catalogs starting from\n"
"                     file:///etc/xml/catalog are activated by default\n\n";

void
trans_usage(int argc, char **argv)
{
    extern const char more_info[];
    extern const char libxslt_more_info[];
    FILE* o = stderr;
    fprintf(o, trans_usage_str);
    fprintf(o, more_info);
    fprintf(o, libxslt_more_info);
    exit(1);
}

/*
 *  This is  main function for 'tr' option
 */

int
xml_trans(int argc, char **argv)
{
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, style;

    int i = 0;
    int errorno = 0;

    if (argc <= 2) trans_usage(argc, argv);

    xmlInitMemory();

    LIBXML_TEST_VERSION

    xmlLineNumbersDefault(1);

    /*
     * Register the EXSLT extensions
     */
    exsltRegisterAll();
    /*
     * Register the test module
    xsltRegisterTestModule();
    */
    
    defaultEntityLoader = xmlGetExternalEntityLoader();
    xmlSetExternalEntityLoader(xslExternalEntityLoader);

    /*
     * Replace entities with their content.
     */
    xmlSubstituteEntitiesDefault(1);

    /*
     * Compile XSLT Sylesheet
     */
    
    {
        style = xmlParseFile((const char *) argv[2]);
        {
            if (style == NULL) {
                fprintf(stderr,  "cannot parse %s\n", argv[2]);
                cur = NULL;
                errorno = 4;
            } else {
                cur = xsltLoadStylesheetPI(style);
                if (cur != NULL) {
                    /* it is an embedded stylesheet */
                    xsltProcess(style, cur, argv[3]);
                    xsltFreeStylesheet(cur);
                    goto done;
                }
                cur = xsltParseStylesheetDoc(style);
                if (cur != NULL) {
                    if (cur->errors != 0) {
                        errorno = 5;
                        goto done;
                    }
                    if (cur->indent == 1)
                        xmlIndentTreeOutput = 1;
                    else
                        xmlIndentTreeOutput = 0;
                } else {
                    xmlFreeDoc(style);
                    errorno = 5;
                    goto done;
                }
            }
        }
    }
    
    /*
     * disable CDATA from being built in the document tree
     */
    xmlDefaultSAXHandlerInit();
    xmlDefaultSAXHandler.cdataBlock = NULL;

    /*
     * run XSLT
     */
    
    if ((cur != NULL) && (cur->errors == 0)) {
        for (i=3; i < argc; i++) {
            doc = NULL;
#ifdef LIBXML_HTML_ENABLED
            if (transOpts.html)
                doc = htmlParseFile(argv[i], NULL);
            else
#endif
#ifdef LIBXML_DOCB_ENABLED
            if (transOpts.docbook)
                doc = docbParseFile(argv[i], NULL);
            else
#endif
                doc = xmlParseFile(argv[i]);
            if (doc == NULL) {
                fprintf(stderr, "unable to parse %s\n", argv[i]);
                errorno = 6;
                continue;
            }
            xsltProcess(doc, cur, argv[i]);
        }

        if (argc == 3)
        {
            /* stdio */
            doc = xmlParseFile("-");
            xsltProcess(doc, cur, "-");
        }
    }
    if (cur != NULL)
        xsltFreeStylesheet(cur);

    for (i = 0; i < nbstrparams; i++)
        xmlFree(strparams[i]);

    /*
     *  Clean up
     */
    
done:
    xsltCleanupGlobals();
    xmlCleanupParser();
#if 0
    xmlMemoryDump();
#endif
    return(errorno);    
}

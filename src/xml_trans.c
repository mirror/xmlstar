/*  $Id: xml_trans.c,v 1.4 2002/11/15 16:35:14 mgrouch Exp $  */

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

/*
 *  This code is based on xsltproc by Daniel Veillard (daniel@veillard.com)
 *  (see also http://xmlsoft.org/)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
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

#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxslt/extensions.h>
#if 0
#include <libxslt/security.h>
#endif

#include <libexslt/exslt.h>

#ifdef LIBXML_DOCB_ENABLED
#include <libxml/DOCBparser.h>
#endif
#ifdef LIBXML_XINCLUDE_ENABLED
#include <libxml/xinclude.h>
#endif

#define MAX_PARAMETERS 256
#define MAX_PATHS 256

void trans_usage(int argc, char **argv)
{
    FILE* o = stderr;

    fprintf(o, "XMLStarlet Toolkit: Transform XML document using XSLT\n");
    fprintf(o, "Usage: xml tr <xsl-file> {-p <name>=<value> } [ <xml-file> ... ]\n");
    fprintf(o, "where\n");
    fprintf(o, "      <xsl-file>         - main XSLT stylesheet for transformation\n");
    fprintf(o, "      <name>=<value>     - name and value of the parameter passed to XSLT processor\n");
    fprintf(o, "      <xml-file>         - input XML document file name (standard input is used if missing)\n\n");

    fprintf(o, "XMLStarlet is a command line toolkit to query/edit/check/transform\n");
    fprintf(o, "XML documents (for more information see http://xmlstar.sourceforge.net/)\n\n");

    fprintf(o, "Current implementation uses libxslt from GNOME codebase as XSLT processor\n");
    fprintf(o, "(see http://xmlsoft.org/ for more details)\n");

    exit(1);
}

static int debug = 0;
static int dumpextensions = 0;
static int novalid = 0;
static int noout = 0;
#ifdef LIBXML_DOCB_ENABLED
static int docbook = 0;
#endif
#ifdef LIBXML_HTML_ENABLED
static int html = 0;
#endif
#ifdef LIBXML_XINCLUDE_ENABLED
static int xinclude = 0;
#endif
static int profile = 0;

static const char *params[MAX_PARAMETERS + 1];
static int nbparams = 0;
static xmlChar *strparams[MAX_PARAMETERS + 1];
static int nbstrparams = 0;
static xmlChar *paths[MAX_PATHS + 1];
static int nbpaths = 0;
static const char *output = NULL;
static int errorno = 0;
static const char *writesubtree = NULL;

xmlExternalEntityLoader defaultEntityLoader = NULL;

static xmlParserInputPtr
xslExternalEntityLoader(const char *URL, const char *ID,
                        xmlParserCtxtPtr ctxt)
{
    xmlParserInputPtr ret;
    warningSAXFunc warning = NULL;

    int i;

    if ((ctxt != NULL) && (ctxt->sax != NULL)) {
        warning = ctxt->sax->warning;
        ctxt->sax->warning = NULL;
    }

    if (defaultEntityLoader != NULL) {
        ret = defaultEntityLoader(URL, ID, ctxt);
        if (ret != NULL) {
            if (warning != NULL)
                ctxt->sax->warning = warning;
            return(ret);
        }
    }
    for (i = 0;i < nbpaths;i++) {
        xmlChar *newURL;
        int len;

        len = xmlStrlen(paths[i]) + xmlStrlen(BAD_CAST URL) + 5;
        newURL = xmlMalloc(len);
        if (newURL != NULL) {
            snprintf(newURL, len, "%s/%s", paths[i], URL);
            ret = defaultEntityLoader((const char *)newURL, ID, ctxt);
            xmlFree(newURL);
            if (ret != NULL) {
                if (warning != NULL)
                    ctxt->sax->warning = warning;
                return(ret);
            }
        }
    }
    if (warning != NULL) {
        ctxt->sax->warning = warning;
        if (URL != NULL)
            warning(ctxt, "failed to load external entity \"%s\"\n", URL);
        else if (ID != NULL)
            warning(ctxt, "failed to load external entity \"%s\"\n", ID);
    }
    return(NULL);
}

static void
xsltProcess(xmlDocPtr doc, xsltStylesheetPtr cur, const char *filename)
{
    xmlDocPtr res;
    xsltTransformContextPtr ctxt;

#ifdef LIBXML_XINCLUDE_ENABLED
    if (xinclude) {
        xmlXIncludeProcess(doc);
    }
#endif
    if (output == NULL) {
        ctxt = xsltNewTransformContext(cur, doc);
        if (ctxt == NULL)
            return;
        if (profile) {
            res = xsltApplyStylesheetUser(cur, doc, params, NULL,
                                          stderr, ctxt);
        } else {
            res = xsltApplyStylesheetUser(cur, doc, params, NULL,
                                          NULL, ctxt);
        }
        if (ctxt->state == XSLT_STATE_ERROR)
            errorno = 9;
        if (ctxt->state == XSLT_STATE_STOPPED)
            errorno = 10;
        xsltFreeTransformContext(ctxt);
        xmlFreeDoc(doc);
        if (res == NULL) {
            fprintf(stderr, "no result for %s\n", filename);
            return;
        }
        if (noout) {
            xmlFreeDoc(res);
            return;
        }

#ifdef LIBXML_DEBUG_ENABLED
        if (debug)
            xmlDebugDumpDocument(stdout, res);
        else {
#endif
            if (cur->methodURI == NULL) {
                xsltSaveResultToFile(stdout, res, cur);                
            } else {
                if (xmlStrEqual
                    (cur->method, (const xmlChar *) "xhtml")) {
                    fprintf(stderr, "non standard output xhtml\n");
                    xsltSaveResultToFile(stdout, res, cur);
                } else {
                    fprintf(stderr,
                            "Unsupported non standard output %s\n",
                            cur->method);
                    errorno = 7;
                }
            }
#ifdef LIBXML_DEBUG_ENABLED
        }
#endif
        xmlFreeDoc(res);
    }
    else {
        int ret;

        ctxt = xsltNewTransformContext(cur, doc);
        if (ctxt == NULL)
            return;
        if (profile) {
            ret = xsltRunStylesheetUser(cur, doc, params, output,
                                        NULL, NULL, stderr, ctxt);
        } else {
            ret = xsltRunStylesheetUser(cur, doc, params, output,
                                        NULL, NULL, stderr, ctxt);
        }
        if (ctxt->state == XSLT_STATE_ERROR)
            errorno = 9;
        xsltFreeTransformContext(ctxt);
        xmlFreeDoc(doc);
    }
}

/*
 * This is like main() function for 'tr' option
 */

int xml_trans(int argc, char **argv)
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
     * Register the EXSLT extensions and the test module
     */
    exsltRegisterAll();
    xsltRegisterTestModule();

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
            if (html)
                doc = htmlParseFile(argv[i], NULL);
            else
#endif
#ifdef LIBXML_DOCB_ENABLED
            if (docbook)
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
    }
    if (cur != NULL)
        xsltFreeStylesheet(cur);

    for (i = 0; i < nbstrparams; i++)
        xmlFree(strparams[i]);
    
done:
    xsltCleanupGlobals();
    xmlCleanupParser();
#if 0
    xmlMemoryDump();
#endif
    return(errorno);    
}

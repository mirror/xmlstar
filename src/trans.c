/*  $Id: trans.c,v 1.1 2002/11/23 23:45:05 mgrouch Exp $  */

/*
 *  This code is based on xsltproc by Daniel Veillard (daniel@veillard.com)
 *  (see also http://xmlsoft.org/)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "trans.h"

/* TODO */

const char *params[MAX_PARAMETERS + 1];
int nbparams = 0;
xmlChar *strparams[MAX_PARAMETERS + 1];
int nbstrparams = 0;
xmlChar *paths[MAX_PATHS + 1];
int nbpaths = 0;
const char *output = NULL;
int errorno = 0;

xmlExternalEntityLoader defaultEntityLoader = NULL;
XmlTrOptions transOpts;

xmlParserInputPtr
xslExternalEntityLoader(const char *URL, const char *ID, xmlParserCtxtPtr ctxt)
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
            snprintf((char *) newURL, len, "%s/%s", paths[i], URL);
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

void
xsltProcess(xmlDocPtr doc, xsltStylesheetPtr cur, const char *filename)
{
    xmlDocPtr res;
    xsltTransformContextPtr ctxt;

#ifdef LIBXML_XINCLUDE_ENABLED
    if (transOpts.xinclude) {
        xmlXIncludeProcess(doc);
    }
#endif
    if (output == NULL) {
        ctxt = xsltNewTransformContext(cur, doc);
        if (ctxt == NULL)
            return;
/*
        if (profile) {
            res = xsltApplyStylesheetUser(cur, doc, params, NULL,
                                          stderr, ctxt);
        } else {
*/
            res = xsltApplyStylesheetUser(cur, doc, params, NULL,
                                          NULL, ctxt);
/*
        }
*/
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
/*
        if (noout) {
            xmlFreeDoc(res);
            return;
        }
*/

/*
#ifdef LIBXML_DEBUG_ENABLED
        if (debug)
            xmlDebugDumpDocument(stdout, res);
        else {
#endif
*/
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
/*
#ifdef LIBXML_DEBUG_ENABLED
        }
#endif
*/
        xmlFreeDoc(res);
    }
    else {
        int ret;

        ctxt = xsltNewTransformContext(cur, doc);
        if (ctxt == NULL)
            return;
/*
        if (profile) {
            ret = xsltRunStylesheetUser(cur, doc, params, output,
                                        NULL, NULL, stderr, ctxt);
        } else {
            ret = xsltRunStylesheetUser(cur, doc, params, output,
                                        NULL, NULL, stderr, ctxt);
        }
*/
        ret = xsltRunStylesheet(cur, doc, params, output, NULL, NULL);
        if (ctxt->state == XSLT_STATE_ERROR)
            errorno = 9;
        xsltFreeTransformContext(ctxt);
        xmlFreeDoc(doc);
    }
}

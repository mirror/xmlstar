/*  $Id: trans.c,v 1.7 2002/11/30 20:29:42 mgrouch Exp $  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "trans.h"

/*
 *  This code is based on xsltproc by Daniel Veillard (daniel@veillard.com)
 *  (see also http://xmlsoft.org/)
 */

/* TODO */
xmlChar *paths[MAX_PATHS + 1];
int nbpaths = 0;

const char *output = NULL;          /* file name to save output */
int errorno = 0;

xmlExternalEntityLoader defaultEntityLoader = NULL;

/**
 *  Initialize global command line options
 */
void
xsltInitOptions(xsltOptionsPtr ops)
{
    ops->noval = 0;
    ops->nonet = 0;
    ops->omit_decl = 0;
    ops->show_extensions = 0;
#ifdef LIBXML_XINCLUDE_ENABLED
    ops->xinclude = 0;
#endif
#ifdef LIBXML_HTML_ENABLED
    ops->html = 0;
#endif
#ifdef LIBXML_DOCB_ENABLED
    ops->docbook = 0;
#endif
#ifdef LIBXML_CATALOG_ENABLED
    ops->catalogs = 0;
#endif
}

/**
 *  Entity loader
 */
xmlParserInputPtr
xsltExternalEntityLoader(const char *URL, const char *ID, xmlParserCtxtPtr ctxt)
{
    xmlParserInputPtr ret;
    warningSAXFunc warning = NULL;

    int i;

    if ((ctxt != NULL) && (ctxt->sax != NULL))
    {
        warning = ctxt->sax->warning;
        ctxt->sax->warning = NULL;
    }

    if (defaultEntityLoader != NULL)
    {
        ret = defaultEntityLoader(URL, ID, ctxt);
        if (ret != NULL)
        {
            if (warning != NULL) ctxt->sax->warning = warning;
            return(ret);
        }
    }

    /* preload resources */
    /* TODO */
    for (i = 0; i < nbpaths; i++)
    {
        xmlChar *newURL;
        int len;

        len = xmlStrlen(paths[i]) + xmlStrlen(BAD_CAST URL) + 5;
        newURL = xmlMalloc(len);
        if (newURL != NULL)
        {
            snprintf((char *) newURL, len, "%s/%s", paths[i], URL);
            ret = defaultEntityLoader((const char *)newURL, ID, ctxt);
            xmlFree(newURL);
            if (ret != NULL)
            {
                if (warning != NULL) ctxt->sax->warning = warning;
                return(ret);
            }
        }
    }

    if (warning != NULL)
    {
        ctxt->sax->warning = warning;
        if (URL != NULL)
            warning(ctxt, "failed to load external entity \"%s\"\n", URL);
        else if (ID != NULL)
            warning(ctxt, "failed to load external entity \"%s\"\n", ID);
    }

    return(NULL);
}

/**
 *  Run stylesheet on XML document
 */
void
xsltProcess(xsltOptionsPtr ops, xmlDocPtr doc, const char** params,
            xsltStylesheetPtr cur, const char *filename)
{
    xmlDocPtr res;
    xsltTransformContextPtr ctxt;

#ifdef LIBXML_XINCLUDE_ENABLED
    if (ops->xinclude) xmlXIncludeProcess(doc);
#endif
    if (output == NULL)
    {
        ctxt = xsltNewTransformContext(cur, doc);
        if (ctxt == NULL) return;
        res = xsltApplyStylesheetUser(cur, doc, params, NULL, NULL, ctxt);
        if (ctxt->state == XSLT_STATE_ERROR)
            errorno = 9;
        if (ctxt->state == XSLT_STATE_STOPPED)
            errorno = 10;
        xsltFreeTransformContext(ctxt);
        xmlFreeDoc(doc);
        if (res == NULL)
        {
            fprintf(stderr, "no result for %s\n", filename);
            return;
        }

        if (cur->methodURI == NULL)
        {
            xsltSaveResultToFile(stdout, res, cur);
        }
        else
        {
            if (xmlStrEqual(cur->method, (const xmlChar *) "xhtml"))
            {
                fprintf(stderr, "non standard output xhtml\n");
                xsltSaveResultToFile(stdout, res, cur);
            }
            else
            {
                fprintf(stderr, "unsupported non standard output %s\n",
                        cur->method);
                errorno = 7;
            }
        }

        xmlFreeDoc(res);
    }
    else
    {
        int ret;

        ctxt = xsltNewTransformContext(cur, doc);
        if (ctxt == NULL) return;

        ret = xsltRunStylesheet(cur, doc, params, output, NULL, NULL);

        if (ctxt->state == XSLT_STATE_ERROR) errorno = 9;

        xsltFreeTransformContext(ctxt);
        xmlFreeDoc(doc);
    }
}

/**
 *  run XSLT on documents
 */
int xsltRun(xsltOptionsPtr ops, char* xsl, const char** params,
            int count, char **docs)
{
    xsltStylesheetPtr cur = NULL;
    xmlDocPtr doc, style;
    int i;

    /*
     * Compile XSLT Sylesheet
     */
    style = xmlParseFile((const char *) xsl);
    if (style == NULL)
    {
        fprintf(stderr,  "cannot parse %s\n", xsl);
        cur = NULL;
        errorno = 4;
    }
    else
    {
        cur = xsltLoadStylesheetPI(style);
        if (cur != NULL)
        {
             /* it is an embedded stylesheet */
             for (i=0; i<count; i++) xsltProcess(ops, style, params, cur, docs[i]);
             xsltFreeStylesheet(cur);
             goto done;
        }

        cur = xsltParseStylesheetDoc(style);
        if (cur != NULL)
        {
            if (cur->errors != 0)
            {
                errorno = 5;
                goto done;
            }
            if (cur->indent == 1) xmlIndentTreeOutput = 1;
            else xmlIndentTreeOutput = 0;
        }
        else
        {
            xmlFreeDoc(style);
            errorno = 5;
            goto done;
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
    if ((cur != NULL) && (cur->errors == 0))
    {
        for (i=0; i<count; i++)
        {
            doc = NULL;
#ifdef LIBXML_HTML_ENABLED
            if (ops->html) doc = htmlParseFile(docs[i], NULL);
            else
#endif
#ifdef LIBXML_DOCB_ENABLED
            if (ops->docbook) doc = docbParseFile(docs[i], NULL);
            else
#endif
                doc = xmlParseFile(docs[i]);
            if (doc == NULL)
            {
                fprintf(stderr, "unable to parse %s\n", docs[i]);
                errorno = 6;
                continue;
            }
            xsltProcess(ops, doc, params, cur, docs[i]);
        }

        if (count == 0)
        {
            /* stdin */
            doc = NULL;
#ifdef LIBXML_HTML_ENABLED
            if (ops->html) doc = htmlParseFile("-", NULL);
            else
#endif
#ifdef LIBXML_DOCB_ENABLED
            if (ops->docbook) doc = docbParseFile("-", NULL);
            else
#endif
                doc = xmlParseFile("-");
            xsltProcess(ops, doc, params, cur, "-");
        }
    }

    /*
     *  Clean up
     */
    if (cur != NULL) xsltFreeStylesheet(cur);

done:
    return(errorno);
}

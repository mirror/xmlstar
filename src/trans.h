/*  $Id: trans.h,v 1.5 2002/11/27 00:09:17 mgrouch Exp $  */

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
#include <libexslt/exslt.h>

#ifdef LIBXML_DOCB_ENABLED
#include <libxml/DOCBparser.h>
#endif
#ifdef LIBXML_XINCLUDE_ENABLED
#include <libxml/xinclude.h>
#endif
#ifdef LIBXML_CATALOG_ENABLED
#include <libxml/catalog.h>
#endif

#define MAX_PARAMETERS 256
#define MAX_PATHS 256

typedef struct _xsltOptions {
    int noval;                /* do not validate against DTDs or schemas */
    int nonet;                /* refuse to fetch DTDs or entities over network */
    int show_extensions;      /* display list of extensions */
#ifdef LIBXML_XINCLUDE_ENABLED
    int xinclude;             /* do XInclude processing on input documents */
#endif
#ifdef LIBXML_HTML_ENABLED
    int html;                 /* inputs are in HTML format */
#endif
#ifdef LIBXML_DOCB_ENABLED
    int docbook;              /* inputs are in SGML docbook format */
#endif
#ifdef LIBXML_CATALOG_ENABLED
    int catalogs;             /* use SGML catalogs from $SGML_CATALOG_FILES */
#endif
} xsltOptions;

typedef xsltOptions *xsltOptionsPtr;

extern xmlExternalEntityLoader defaultEntityLoader;

/* TODO */
extern xmlChar *paths[MAX_PATHS + 1];
extern int nbpaths;

extern int errorno;

void xsltInitOptions(xsltOptionsPtr ops);

void xsltProcess(xsltOptionsPtr ops, xmlDocPtr doc,
                 const char **params, xsltStylesheetPtr cur,
                 const char *filename);

int xsltRun(xsltOptionsPtr ops, char* xsl,
            const char **params,
            int count, char **docs);

xmlParserInputPtr xsltExternalEntityLoader(const char *URL,
                                           const char *ID,
                                           xmlParserCtxtPtr ctxt);

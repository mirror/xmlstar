/*  $Id: trans.h,v 1.1 2002/11/23 23:45:05 mgrouch Exp $  */

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

typedef struct _XmlTrOptions {
#ifdef LIBXML_XINCLUDE_ENABLED
    int xinclude;             /* do XInclude processing on input documents */
#endif
#if 1
    int novalid;              /* do not validate against DTDs or schemas */
#endif
#if 1
    int nonet;                /* refuse to fetch DTDs or entities over network */
#endif
#ifdef LIBXML_DOCB_ENABLED
    int docbook;              /* inputs are in SGML docbook format */
#endif
#ifdef LIBXML_HTML_ENABLED
    int html;                 /* inputs are in HTML format */
#endif
#if 1
    int show_extensions;      /* display list of extensions */
#endif
} XmlTrOptions;


extern XmlTrOptions transOpts;
extern xmlExternalEntityLoader defaultEntityLoader;

extern const char *params[MAX_PARAMETERS + 1];
extern int nbparams;
extern xmlChar *strparams[MAX_PARAMETERS + 1];
extern int nbstrparams;
/*
extern xmlChar *paths[MAX_PATHS + 1];
extern int nbpaths;
*/
extern int errorno;


xmlParserInputPtr xslExternalEntityLoader(const char *URL, const char *ID,
                                          xmlParserCtxtPtr ctxt);

void xsltProcess(xmlDocPtr doc, xsltStylesheetPtr cur, const char *filename);

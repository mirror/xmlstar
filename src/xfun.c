#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/tree.h>
#include <libexslt/exslt.h>

#include "process.h"

static void
exec(xmlXPathParserContextPtr ctxt, int nargs)
{
    ProcPtr proc;
    xmlBufferPtr buffer;
    int nread, i;

    char **argv = xmlMalloc((nargs+1) * sizeof(char*));
    for (i = 0; i < nargs; i++) {
        xmlChar *arg = xmlXPathPopString(ctxt);
        argv[nargs-i-1] = (char*) arg;
    }
    argv[nargs] = NULL;

    proc = proc_spawn(argv);

    for (i = 0; i < nargs; i++) {
        xmlFree(argv[i]);
    }
    xmlFree(argv);

    buffer = xmlBufferCreate();
    for (;;) {
        xmlChar buf[4096];
        nread = proc_read(proc, buf, sizeof buf);
        if (nread <= 0) break;
        xmlBufferAdd(buffer, buf, nread);
    }
    proc_wait(proc);
    xmlXPathReturnString(ctxt, xmlStrdup(xmlBufferContent(buffer)));
    xmlBufferFree(buffer);
}

void
register_xstar_funs(xmlXPathContextPtr ctxt)
{
    const xmlChar *XMLSTAR_NS = BAD_CAST "http://xmlstar.sourceforge.net";
    const xmlChar *XMLSTAR_PREFIX = BAD_CAST "xstar";

    xmlXPathRegisterNs(ctxt, XMLSTAR_PREFIX, XMLSTAR_NS);
    xmlXPathRegisterFuncNS(ctxt, BAD_CAST "exec", XMLSTAR_NS, exec);
}

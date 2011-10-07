#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/tree.h>
#include <libexslt/exslt.h>

#include "process.h"

/* like xmlXPathStackIsNodeSet, but for any XPathObject */
static int
isNodeSet(xmlXPathObjectPtr xpath_obj)
{
    return xpath_obj != NULL &&
        (xpath_obj->type == XPATH_NODESET ||
            xpath_obj->type == XPATH_XSLT_TREE);
}

static void
exec(xmlXPathParserContextPtr ctxt, int nargs)
{
    ProcPtr proc;
    xmlBufferPtr buffer;
    int nread, i, j, argc, argi;
    char **argv;

    /* count the arguments */
    for (i = 0; i < nargs; i++) {
        xmlXPathObjectPtr val = ctxt->valueTab[i];
        if (isNodeSet(val)) {
            argc += val->nodesetval->nodeNr;
        } else {
            argc += 1;
        }
    }

    /* collect arguments in argv */
    argv = xmlMalloc((argc+1) * sizeof(char*));
    /* NOTE: the last argument is on top of the stack, ie backwards */
    for (i = 0, argi = argc; i < nargs; i++) {
        if (xmlXPathStackIsNodeSet(ctxt)) {
            xmlNodeSetPtr args = xmlXPathPopNodeSet(ctxt);
            argi -= args->nodeNr;
            /* NOTE: but for node sets the arguments are in order */
            for (j = 0; j < args->nodeNr; j++) {
                argv[argi + j] =
                    (char*) xmlXPathCastNodeToString(args->nodeTab[j]);
            }
            xmlXPathFreeNodeSet(args);
        } else {
            xmlChar *arg = xmlXPathPopString(ctxt);
            argi -= 1;
            argv[argi] = (char*) arg;
        }
    }
    argv[argc] = NULL;

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

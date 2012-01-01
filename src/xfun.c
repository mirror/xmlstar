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

static void
readline(xmlXPathParserContextPtr ctxt, int nargs)
{
    ProcPtr proc = xmlXPathPopExternal(ctxt);
    xmlBufferPtr buffer;

    buffer = xmlBufferCreate();
    for (;;) {
        /* TODO: probably should be buffered... */
        xmlChar buf[1];
        int nread = proc_read(proc, buf, sizeof buf);
        if (nread <= 0 || buf[0] == '\n') break;
        xmlBufferAdd(buffer, buf, nread);
    }
    xmlXPathReturnString(ctxt, xmlStrdup(xmlBufferContent(buffer)));
    xmlBufferFree(buffer);
}

static void
filterline(xmlXPathParserContextPtr ctxt, int nargs)
{
    xmlChar *str = xmlXPathPopString(ctxt);
    ProcPtr proc = xmlXPathPopExternal(ctxt);
    int len = xmlStrlen(str);

    proc_write(proc, str, len);
    if (str[len-1] != '\n') {
        str[0] = '\n';
        proc_write(proc, str, 1);
    }
    xmlFree(str);

    valuePush(ctxt, xmlXPathWrapExternal(proc));
    readline(ctxt, 1);
}

static const xmlChar *XMLSTAR_NS = BAD_CAST "http://xmlstar.sourceforge.net";
static const xmlChar *XMLSTAR_PREFIX = BAD_CAST "xstar";

void
register_xstar_funs(xmlXPathContextPtr ctxt)
{
    xmlXPathRegisterNs(ctxt, XMLSTAR_PREFIX, XMLSTAR_NS);
    xmlXPathRegisterFuncNS(ctxt, BAD_CAST "exec", XMLSTAR_NS, &exec);
    xmlXPathRegisterFuncNS(ctxt, BAD_CAST "readline", XMLSTAR_NS, &readline);
    xmlXPathRegisterFuncNS(ctxt, BAD_CAST "filterline", XMLSTAR_NS, &filterline);
}

void
register_proc(void *argv_payload, void *ctxt_data, xmlChar *name)
{
    char **argv = argv_payload;
    xmlXPathContextPtr ctxt = ctxt_data;
    ProcPtr proc = proc_spawn(argv);
    xmlXPathRegisterVariableNS(ctxt, name, XMLSTAR_NS, xmlXPathWrapExternal(proc));
}

void
release_proc(void *payload, void *ctxt_data, xmlChar *name)
{
    xmlXPathContextPtr ctxt = ctxt_data;
    xmlXPathObjectPtr val = xmlXPathVariableLookupNS(ctxt, name, XMLSTAR_NS);
    ProcPtr proc = val->user;
    proc_wait(proc);
    xmlXPathFreeObject(val);
}

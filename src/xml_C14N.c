/*
 *  $Id: xml_C14N.c,v 1.12 2004/11/24 03:00:10 mgrouch Exp $
 *
 *  Canonical XML implementation test program
 *  (http://www.w3.org/TR/2001/REC-xml-c14n-20010315)
 *
 *  See Copyright for the status of this software.
 * 
 *  Author: Aleksey Sanin <aleksey@aleksey.com>
 */

#include <libxml/xmlversion.h>
#include "config.h"

#if defined(LIBXML_C14N_ENABLED)

#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <libxml/c14n.h>



static const char c14n_usage_str_1[] =
"XMLStarlet Toolkit: XML canonicalization\n"
"Usage: xml c14n <mode> <xml-file> [<xpath-file>] [<inclusive-ns-list>]\n"
"where\n"
"  <xml-file>   - input XML document file name (stdin is used if '-')\n"
"  <xpath-file> - XML file containing XPath expression for\n"
"                 c14n XML canonicalization\n";

static const char c14n_usage_str_2[] =
"    Example:\n"
"    <?xml version=\"1.0\"?>\n"
"    <XPath xmlns:n0=\"http://a.example.com\" xmlns:n1=\"http://b.example\">\n"
"    (//. | //@* | //namespace::*)[ancestor-or-self::n1:elem1]\n"
"    </XPath>\n"
"\n"
"  <inclusive-ns-list> - the list of inclusive namespace prefixes\n"
"                        (only for exclusive canonicalization)\n"
"    Example: 'n1 n2'\n"
"\n";

static const char c14n_usage_str_3[] =
"  <mode> is one of following:\n"
"  --with-comments         XML file canonicalization w comments (default)\n"
"  --without-comments      XML file canonicalization w/o comments\n"
"  --exc-with-comments     Exclusive XML file canonicalization w comments\n"
"  --exc-without-comments  Exclusive XML file canonicalization w/o comments\n"
"\n";

static void c14nUsage(const char *name)
{
    extern const char more_info[];
    FILE* o = stderr;
    fprintf(o, "%s", c14n_usage_str_1);
    fprintf(o, "%s", c14n_usage_str_2);
    fprintf(o, "%s", c14n_usage_str_3);
    fprintf(o, "%s", more_info);
    exit(1);
}

static xmlXPathObjectPtr
load_xpath_expr (xmlDocPtr parent_doc, const char* filename);

static xmlChar **parse_list(xmlChar *str);

#if 0
static void print_xpath_nodes(xmlNodeSetPtr nodes);
#endif

static int 
run_c14n(const char* xml_filename, int with_comments, int exclusive,
         const char* xpath_filename, xmlChar **inclusive_namespaces) {
    xmlDocPtr doc;
    xmlXPathObjectPtr xpath = NULL; 
    xmlChar *result = NULL;
    int ret;
    xmlExternalEntityLoader defaultEntityLoader = NULL;

    /*
     * build an XML tree from a the file; we need to add default
     * attributes and resolve all character and entities references
     */
    xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;
    xmlSubstituteEntitiesDefault(1);

    /*
     * Do not fetch DTD over network
     */
    defaultEntityLoader = xmlNoNetExternalEntityLoader;
    xmlSetExternalEntityLoader(xmlNoNetExternalEntityLoader);
    xmlLoadExtDtdDefaultValue = 0;
       
    doc = xmlParseFile(xml_filename);
    if (doc == NULL) {
        fprintf(stderr, "Error: unable to parse file \"%s\"\n", xml_filename);
        return(-1);
    }
    
    /*
     * Check the document is of the right kind
     */    
    if(xmlDocGetRootElement(doc) == NULL) {
        fprintf(stderr,"Error: empty document for file \"%s\"\n", xml_filename);
        xmlFreeDoc(doc);
        return(-1);
    }

    /* 
     * load xpath file if specified 
     */
    if(xpath_filename) {
        xpath = load_xpath_expr(doc, xpath_filename);
        if(xpath == NULL) {
            fprintf(stderr,"Error: unable to evaluate xpath expression\n");
            xmlFreeDoc(doc); 
            return(-1);
        }
    }

    /*
     * Canonical form
     */      
    /* fprintf(stderr,"File \"%s\" loaded: start canonization\n", xml_filename); */
    ret = xmlC14NDocDumpMemory(doc, 
            (xpath) ? xpath->nodesetval : NULL, 
            exclusive, inclusive_namespaces,
            with_comments, &result);
    if(ret >= 0) {
        if(result != NULL) {
            write(1, result, ret);
            fflush(NULL);
            xmlFree(result);
        }
    } else {
        fprintf(stderr,"Error: failed to canonicalize XML file \"%s\" (ret=%d)\n", xml_filename, ret);
        if(result != NULL) xmlFree(result);
        xmlFreeDoc(doc);
        return(-1);
    }
 
    /*
     * Cleanup
     */ 
    if(xpath != NULL) xmlXPathFreeObject(xpath);
    xmlFreeDoc(doc);    

    return(ret);
}

int c14nMain(int argc, char **argv) {
    int ret = -1;
    
    /*
     * Init libxml
     */     
    xmlInitParser();
    LIBXML_TEST_VERSION
    
    /*
     * Parse command line and process file
     */
    if (argc < 4) {
        if (argc >= 3)
        {
            if ((!strcmp(argv[2], "--help")) || (!strcmp(argv[2], "-h")))
                c14nUsage(argv[1]);
        }
        ret = run_c14n((argc > 2)? argv[2] : "-", 1, 0, NULL, NULL);
    } else if(strcmp(argv[2], "--with-comments") == 0) {
        ret = run_c14n(argv[3], 1, 0, (argc > 4) ? argv[4] : NULL, NULL);
    } else if(strcmp(argv[2], "--without-comments") == 0) {
        ret = run_c14n(argv[3], 0, 0, (argc > 4) ? argv[4] : NULL, NULL);
    } else if(strcmp(argv[2], "--exc-with-comments") == 0) {
        xmlChar **list;
        
        /* load exclusive namespace from command line */
        list = (argc > 5) ? parse_list((xmlChar *)argv[5]) : NULL;
        ret = run_c14n(argv[3], 1, 1, (argc > 4) ? argv[4] : NULL, list);
        if(list != NULL) xmlFree(list);
    } else if(strcmp(argv[2], "--exc-without-comments") == 0) {
        xmlChar **list;
        
        /* load exclusive namespace from command line */
        list = (argc > 5) ? parse_list((xmlChar *)argv[5]) : NULL;
        ret = run_c14n(argv[3], 0, 1, (argc > 4) ? argv[4] : NULL, list);
        if(list != NULL) xmlFree(list);
    } else {
        fprintf(stderr, "error: bad arguments.\n");
        c14nUsage(argv[1]);
    }

    /* 
     * Shutdown libxml
     */
    xmlCleanupParser();
    xmlMemoryDump();
    
    return ((ret >= 0) ? 0 : 1);
}

/*
 * Macro used to grow the current buffer.
 */
#define growBufferReentrant() {                                         \
    buffer_size *= 2;                                                   \
    buffer = (xmlChar **)                                               \
                    xmlRealloc(buffer, buffer_size * sizeof(xmlChar*)); \
    if (buffer == NULL) {                                               \
        perror("realloc failed");                                       \
        return(NULL);                                                   \
    }                                                                   \
}

static xmlChar **
parse_list(xmlChar *str) {
    xmlChar **buffer;
    xmlChar **out = NULL;
    int buffer_size = 0;
    int len;

    if(str == NULL) {
        return(NULL);
    }

    len = xmlStrlen(str);
    if((str[0] == '\'') && (str[len - 1] == '\'')) {
        str[len - 1] = '\0';
        str++;
        len -= 2;
    }
    /*
     * allocate an translation buffer.
     */
    buffer_size = 1000;
    buffer = (xmlChar **) xmlMalloc(buffer_size * sizeof(xmlChar*));
    if (buffer == NULL) {
        perror("malloc failed");
        return(NULL);
    }
    out = buffer;
    
    while(*str != '\0') {
        if (out - buffer > buffer_size - 10) {
            int indx = out - buffer;

            growBufferReentrant();
            out = &buffer[indx];
        }
        (*out++) = str;
        while(*str != ',' && *str != '\0') ++str;
        if(*str == ',') *(str++) = '\0';
    }
    (*out) = NULL;
    return buffer;
}

static xmlXPathObjectPtr
load_xpath_expr (xmlDocPtr parent_doc, const char* filename) {
    xmlXPathObjectPtr xpath; 
    xmlDocPtr doc;
    xmlChar *expr;
    xmlXPathContextPtr ctx; 
    xmlNodePtr node;
    xmlNsPtr ns;
    
    /*
     * load XPath expr as a file
     */
    xmlLoadExtDtdDefaultValue = XML_DETECT_IDS | XML_COMPLETE_ATTRS;
    xmlSubstituteEntitiesDefault(1);

    doc = xmlParseFile(filename);
    if (doc == NULL) {
        fprintf(stderr, "Error: unable to parse file \"%s\"\n", filename);
        return(NULL);
    }
    
    /*
     * Check the document is of the right kind
     */    
    if(xmlDocGetRootElement(doc) == NULL) {
        fprintf(stderr,"Error: empty document for file \"%s\"\n", filename);
        xmlFreeDoc(doc);
        return(NULL);
    }

    node = doc->children;
    while(node != NULL && !xmlStrEqual(node->name, (const xmlChar *)"XPath")) {
        node = node->next;
    }
    
    if(node == NULL) {   
        fprintf(stderr,"Error: XPath element expected in the file  \"%s\"\n", filename);
        xmlFreeDoc(doc);
        return(NULL);
    }

    expr = xmlNodeGetContent(node);
    if(expr == NULL) {
        fprintf(stderr,"Error: XPath content element is NULL \"%s\"\n", filename);
        xmlFreeDoc(doc);
        return(NULL);
    }

    ctx = xmlXPathNewContext(parent_doc);
    if(ctx == NULL) {
        fprintf(stderr,"Error: unable to create new context\n");
        xmlFree(expr); 
        xmlFreeDoc(doc); 
        return(NULL);
    }

    /*
     * Register namespaces
     */
    ns = node->nsDef;
    while(ns != NULL) {
        if(xmlXPathRegisterNs(ctx, ns->prefix, ns->href) != 0) {
            fprintf(stderr,"Error: unable to register NS with prefix=\"%s\" and href=\"%s\"\n", ns->prefix, ns->href);
                xmlFree(expr); 
            xmlXPathFreeContext(ctx); 
            xmlFreeDoc(doc); 
            return(NULL);
        }
        ns = ns->next;
    }

    /*  
     * Evaluate xpath
     */
    xpath = xmlXPathEvalExpression(expr, ctx);
    if(xpath == NULL) {
        fprintf(stderr,"Error: unable to evaluate xpath expression\n");
            xmlFree(expr); 
        xmlXPathFreeContext(ctx); 
        xmlFreeDoc(doc); 
        return(NULL);
    }

    /* print_xpath_nodes(xpath->nodesetval); */

    xmlFree(expr); 
    xmlXPathFreeContext(ctx); 
    xmlFreeDoc(doc); 
    return(xpath);
}

#if 0
static void
print_xpath_nodes(xmlNodeSetPtr nodes) {
    xmlNodePtr cur;
    int i;
    
    if(nodes == NULL ){ 
        fprintf(stderr, "Error: no nodes set defined\n");
        return;
    }
    
    fprintf(stderr, "Nodes Set:\n-----\n");
    for(i = 0; i < nodes->nodeNr; ++i) {
        if(nodes->nodeTab[i]->type == XML_NAMESPACE_DECL) {
            xmlNsPtr ns;
            
            ns = (xmlNsPtr)nodes->nodeTab[i];
            cur = (xmlNodePtr)ns->next;
            fprintf(stderr, "namespace \"%s\"=\"%s\" for node %s:%s\n", 
                    ns->prefix, ns->href,
                    (cur->ns) ? cur->ns->prefix : BAD_CAST "", cur->name);
        } else if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
            cur = nodes->nodeTab[i];    
            fprintf(stderr, "element node \"%s:%s\"\n", 
                    (cur->ns) ? cur->ns->prefix : BAD_CAST "", cur->name);
        } else {
            cur = nodes->nodeTab[i];    
            fprintf(stderr, "node \"%s\": type %d\n", cur->name, cur->type);
        }
    }
}
#endif

#else
#include <stdio.h>
int c14nMain(int argc, char **argv) {
    printf("%s : XPath/Canonicalization support not compiled in\n", argv[0]);
    return 2;
}
#endif /* LIBXML_C14N_ENABLED */


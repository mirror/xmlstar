#ifndef XFUN_H
#define XFUN_H

void register_xstar_funs(xmlXPathContextPtr ctxt);

/* xmlHashScanner functions */
void register_proc(void *argv_payload, void *ctxt_data, xmlChar *name);
void release_proc(void *proc_payload, void *data, xmlChar *name);



#endif  /* XFUN_H */

/* $Id: binsert.h,v 1.1 2003/05/11 04:53:56 mgrouch Exp $ */

#ifndef __BINSERT_H
#define __BINSERT_H

#include <libxml/xmlstring.h>

typedef struct _SortedArrayObj SortedArrayObj;
typedef SortedArrayObj*  SortedArray;
typedef xmlChar* ArrayItem;

SortedArray array_create();
void array_free(SortedArray array);
int array_binary_insert(SortedArray array, ArrayItem item);
ArrayItem array_item(SortedArray array, int idx);
int array_len(SortedArray array);

#endif /* __BINSERT_H */

/* $Id: binsert.c,v 1.2 2003/05/11 17:28:18 mgrouch Exp $ */

#include "binsert.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <libxml/xmlmemory.h>

#ifndef NULL
#define NULL 0
#endif

#define ARR_INIT_SZ  256
#define ARR_INCR_SZ  256

struct _SortedArrayObj
{
   ArrayItem* data;
   int        size;
   int        len;
};

SortedArray
array_create()
{
   SortedArray array;

   array = xmlMalloc(sizeof(SortedArrayObj));
   array->data = xmlMalloc(ARR_INIT_SZ * sizeof(ArrayItem));
   array->size = ARR_INIT_SZ;
   array->len = 0;
   
   return array; 
}

void
array_free(SortedArray array)
{
   free(array->data);
   array->data = NULL;
   array->size = 0;
   array->len = 0;
   free(array);
}

ArrayItem
array_item(SortedArray array, int idx)
{
   return array->data[idx];
}

int
array_len(SortedArray array)
{
   return array->len;
}

int
array_binary_insert(SortedArray array, ArrayItem item)
{
   int bottom = 0, middle, top = array->len - 1;
   int compare_result;
   int found = -1;
   
   ArrayItem middle_item;

   while(bottom <= top)
   {
      middle = (top + bottom) / 2;
      middle_item = array->data[middle];

      compare_result = xmlStrcmp(item, middle_item);
      if(compare_result > 0)
          bottom = middle + 1;
      else if(compare_result < 0)
          top = middle - 1;
      else
      {
          found = middle;
          break;
      }
   }

   /* Insert only unique items */
   if (found < 0)
   {
      int move_len;

#if 0      
      printf("len %d  bottom: %d   top: %d    item: %s\n", array->len, bottom, top, item);
#endif

      if (array->size <= (array->len + 1))
      {
          ArrayItem* new_data;
          new_data = realloc(array->data, (array->size + ARR_INCR_SZ) * sizeof(ArrayItem));
          array->data = new_data;
          array->size += ARR_INCR_SZ;
      }

      move_len = array->len - bottom;
      if (move_len > 0)
      {
          memmove(array->data + bottom + 1, array->data + bottom, move_len * sizeof(ArrayItem));
      }
       
      array->data[bottom] = item;
      array->len += 1;

      return bottom;
   }

   return -1;
}

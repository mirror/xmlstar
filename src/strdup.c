/* $Id: strdup.c,v 1.1 2003/12/17 06:26:01 mgrouch Exp $ */ 

/*

XMLStarlet: Command Line Toolkit to query/edit/check/transform XML documents

Copyright (c) 2002 Mikhail Grushinskiy.  All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <stdlib.h>
#include <string.h>
#include "strdup.h"


/*
 *  ISO compliant strdup implementation
 */

char *
strdup(const char *cur)
{
	size_t len;
	char *ret;

	if (cur == NULL)
		return (NULL);

	len = strlen(cur) + 1;
	ret = malloc(len);
	if (ret == NULL)
		return (NULL);

	memcpy(ret, cur, len);
	return (ret);
}

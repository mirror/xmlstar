/* $Id: stack.c,v 1.1 2003/03/19 23:18:06 mgrouch Exp $ */

#include "stack.h"
#include <stdlib.h>

#ifndef NULL
#define NULL 0
#endif

/**
 *   Very simple stack implementation for now
 */
 
struct _StackObj
{
    char *items;           /* stack content */
    int   depth;           /* stack current depth */
    int   max_depth;       /* stack max depth */
};

/**
 *   Stack methods
 */

Stack
stack_create(int max_depth)
{
    Stack stack = NULL;

    stack = (Stack) malloc(sizeof(Stack));
    stack->items = 0;
    stack->depth = 0;
    stack->items = (StackItem *) malloc(max_depth);
    stack->max_depth = max_depth;

    return stack;
}

void
stack_free(Stack stack)
{
    free(stack->items);
    stack->items = 0;
    stack->depth = 0;
    stack->max_depth = 0;
    free(stack);
}
      
int
stack_push(Stack stack, StackItem item)
{
    if (stack->depth < stack->max_depth - 1)
    {
        stack->items[stack->depth] = item;
        stack->depth++;
        return 0;
    }
    else
        return -1;
}

StackItem
stack_pop(Stack stack)
{
    stack->depth--;
    return stack->items[stack->depth];
}

int
stack_depth(Stack stack)
{
    return stack->depth;
}

int
stack_isEmpty(Stack stack)
{
    return stack->depth? 0: 1;
}

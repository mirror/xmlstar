/* $Id: stack.h,v 1.1 2003/03/19 23:18:06 mgrouch Exp $ */

#ifndef __STACK_H
#define __STACK_H

typedef struct _StackObj StackObj;
typedef StackObj*  Stack;
typedef char StackItem;

Stack stack_create(int max_depth);

void stack_free(Stack stack);

int stack_push(Stack stack, StackItem item);

StackItem stack_pop(Stack stack);

int stack_depth(Stack stack);

int stack_isEmpty(Stack stack);

#endif /* __STACK_H */

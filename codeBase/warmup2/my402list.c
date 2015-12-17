#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>

#include "cs402.h"

#include "my402list.h"

int  My402ListLength(My402List* inList)
{
	return inList->num_members;
}

int  My402ListEmpty(My402List* inList)
{

	if(inList->num_members == 0)
        {
     		return 1;
        }
  	else
        {
     		return 0;
        }
}

int  My402ListAppend(My402List* inList, void* obj)
{
	if(inList==NULL)
	{
		return 0;
	}
	else{
		My402ListElem *node;	
		node = (My402ListElem *)malloc(sizeof(My402ListElem));
		node->obj = obj;
		if(My402ListEmpty(inList))
		{
			inList->anchor.next=node;
			inList->anchor.prev=node;
			node->next=&(inList->anchor);
			node->prev=&(inList->anchor);
		}
		else
		{
			node->next = &(inList->anchor);
			node->prev = inList->anchor.prev;
			inList->anchor.prev->next=node;	
			inList->anchor.prev = node;		
		}
	inList->num_members++;
	return 1;
	}
}

int  My402ListPrepend(My402List* inList, void* obj)
{
	if(inList==NULL){
		return 0;
	}
	else{
		My402ListElem *node;
		node = (My402ListElem *)malloc(sizeof(My402ListElem));
		node->obj = obj;
		if(My402ListEmpty(inList))
		{
			inList->anchor.next=node;
			inList->anchor.prev=node;
			node->next=&(inList->anchor);
			node->prev=&(inList->anchor);
		}
		else
		{
			node->next = inList->anchor.next;
			node->prev = &(inList->anchor);
			inList->anchor.next->prev = node;
			inList->anchor.next = node;
		}
	inList->num_members++;
	return 1;
	}
	
}

void My402ListUnlink(My402List* inList, My402ListElem* node)
{
	node->next->prev = node->prev;
	node->prev->next = node->next;
	node->next=NULL;
	node->prev=NULL;
	inList->num_members--;
	free(node);
}

void My402ListUnlinkAll(My402List* inList)
{
	My402ListElem *node,*temp=NULL;
	node = My402ListFirst(inList);
	while(node != &(inList->anchor))
	{
		temp=node->next;
		My402ListUnlink(inList,node);
		node=temp;
	}
	inList->anchor.next = &(inList->anchor);
	inList->anchor.prev = &(inList->anchor);
}
int  My402ListInsertAfter(My402List* inList, void* obj, My402ListElem* node)
{
	if(inList == NULL)
	{
		return 0;
	}
	else
	{
		My402ListElem *newNode;
		newNode = (My402ListElem*)malloc(sizeof(My402ListElem));
		newNode->obj = obj;
		newNode->next=node->next;
		node->next->prev=newNode;
		node->next = newNode;
		newNode->prev = node;
		inList->num_members++;
		return 1;
	}
	
}
int  My402ListInsertBefore(My402List* inList, void* obj, My402ListElem* node)
{
	if(inList==NULL)
	{
		return 0;
	}
	else
	{
		My402ListElem *newNode;
		newNode =(My402ListElem*)malloc(sizeof(My402ListElem));
		newNode->obj = obj;
		newNode->prev=node->prev;
		node->prev->next=newNode;
		node->prev=newNode;
		newNode->next=node;
		inList->num_members++;
		return 1;
	}
	
}

My402ListElem *My402ListFirst(My402List* inList)
{
	if(inList==NULL)
	{
		return NULL;
	}
	if(My402ListEmpty(inList))
	{
		return NULL;
	}
  return inList->anchor.next; 
}
My402ListElem *My402ListLast(My402List* inList)
{
	if(inList==NULL)
	{
		return NULL;
	}
	if(My402ListEmpty(inList))
	{
		return NULL;
	}

  return inList->anchor.prev;
}
My402ListElem *My402ListNext(My402List* inList, My402ListElem* node)
{
	if(inList==NULL)
	{
		return NULL;
	}
	if(My402ListEmpty(inList))
	{
		return NULL;
	}
	if(node->next==&(inList->anchor))
	{
		return NULL;
	}
  return node->next;
}
My402ListElem *My402ListPrev(My402List* inList, My402ListElem* node)
{
	if(inList==NULL)
	{
		return NULL;
	}
	if(My402ListEmpty(inList))
	{
		return NULL;
	}
	if(node->prev==&(inList->anchor))
	{
		return NULL;
	}
  return node->prev;
}
My402ListElem *My402ListFind(My402List* inList, void* obj)
{
	My402ListElem *node=NULL;
	for (node=My402ListFirst(inList);node!=NULL;node=My402ListNext(inList,node))
	{
		if(node->obj==obj)
		{
			return node;
		}
	}
	return NULL;
}

int My402ListInit(My402List* inList)
{
	if(inList==NULL)
	{
		return 0;
	}
	else
	{
		inList->num_members = 0;
		inList->anchor.next=&(inList->anchor);
		inList->anchor.prev=&(inList->anchor);
		inList->anchor.obj=NULL;
		return 1;
	}
}


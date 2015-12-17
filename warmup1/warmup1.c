#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include "my402list.h"

struct nodeVariables{
	
	char symbol[5];
	int transTime;
	char transDescription[100];
	char convertedTime[100];
	char transAmt[100];
	char printableAmt[100];
};

static double balance = 0;

struct nodeVariables tFileObjects[1000];
double doTransaction (My402ListElem *);
char* addCommas(char *);
int checkAmount(char *);

My402List pList;

void readFile(int inArg,char * inFile){
	My402ListInit(&pList);
	char line[1026];
	time_t tempTime;
	char timeBuffer[50];
	struct tm * timeinfo;
	int i=0;
	int j;
	char tabChar_count[1026];
	int tab_count=0;
	time_t current_time;
	FILE *file = NULL;
	struct stat stat_var;
	lstat(inFile, &stat_var);
	if(S_ISDIR(stat_var.st_mode)){
	printf("\nThe given input is a directory , Malformed Input\n");
        	exit(1);
    }

	if(inArg==2){
		file=stdin;
	}else if(inArg==3){
		if(fopen(inFile,"r"))
		{
			file=fopen(inFile,"r");
		}else if(!(access (inFile, F_OK)==0)){
			printf("\nThe file dosent exist or access denied !!, Malformed Command\n");
			exit(1);
		}
		if(!(access(inFile, R_OK) == 0))
		{
		    printf("\nPermission Denied, Malformed Command !\n");
		    exit(1);
		}	
	}else{
		printf("\nThere is an error, Invalid Arguments\n");
		exit(1);
	}
	
	if(file==NULL)
		printf("\nThe file is empty and there is error in reading the file\n");
	else
	{
		while (fgets(line, sizeof(line), file)) {
			tab_count=0;
			if(strlen(line)>1024){
				printf("\nMalformed Input, pleae check the number of characters in the line\n");
				exit(1);
			}
			strcpy(tabChar_count,line);
			for(j=0;tabChar_count[j] !='\0';j++){
				if(tabChar_count[j]=='\t'){
					tab_count++;
				}
			}
			if(tab_count!=3){
					printf("\nMalformed Input, please check the format of the line\n");
					exit(1);
			}
			sscanf(line,"%s\t%d\t%s\t%[^\n]",tFileObjects[i].symbol,&tFileObjects[i].transTime,tFileObjects[i].transAmt,tFileObjects[i].transDescription);
			tempTime = (time_t)tFileObjects[i].transTime;
			if(!(strcmp(tFileObjects[i].symbol,"+")==0 || strcmp(tFileObjects[i].symbol,"-")==0)){
				printf("\nMalformed Input, please check the symbols\n");
				exit(1);
			}
			current_time = time(NULL);
			if(tempTime > current_time){
				printf("\nMalformed Input, please check the timeStamp\n");
				exit(1);
			}
			if(!checkAmount(tFileObjects[i].transAmt)){
				printf("\nMalformed Input, please check the format of Amount\n");
				exit(1);
			}
			timeinfo = localtime(&tempTime);
			strftime(timeBuffer,16,"%a %b %e %Y",timeinfo);
			strcpy(tFileObjects[i].convertedTime,timeBuffer);
			My402ListAppend(&pList, &tFileObjects[i]);
			i++;
		}
	}
}

int checkAmount(char *input)
{
	char tempChar[30];
	char tk_1[10];
	char tk_2[10];
	strcpy(tempChar,input);
	strcpy(tk_1,strtok(tempChar,"."));
	strcpy(tk_2,strtok(NULL,"."));
	if(strlen(tk_2)>2){
		return 0;
	}
	else 
		return 1;	
}

void printList(My402List *node){
	My402ListElem *elem=NULL;
	double temp = 0;
	struct nodeVariables *ptr = NULL;
	char token_part1[10];
	char token_part2[10];
	char addedCommas[30];
	char tempChar[30];
	printf("+-----------------+--------------------------+----------------+----------------+");
    printf("\n|       Date      | Description              |         Amount |        Balance |");
   	printf("\n+-----------------+--------------------------+----------------+----------------+");
	for (elem=My402ListFirst(node);elem!=NULL;elem=My402ListNext(node,elem)){
		ptr = elem->obj;
		printf("\n| %s | %-24.24s |",ptr->convertedTime,ptr->transDescription);
		if(strcmp(ptr->symbol,"+")==0){
			printf(" %13s  |",ptr->printableAmt);
		}
		else if(strcmp(ptr->symbol,"-")==0){
			printf(" (%12s) |",ptr->printableAmt);
		}
		temp = doTransaction(elem);
		if(temp > 10000000){
			printf(" %13.12s  |","?,???,???.??");
		}
		else if(temp<0){
			temp=-(temp);
			snprintf(tempChar,50,"%lf",temp);
			strcpy(token_part1,strtok(tempChar,"."));
			strcpy(token_part2,strtok(NULL,"."));
			token_part2[2]='\0';
			strcpy(addedCommas,addCommas(token_part1));
			strcat(addedCommas,".");
			strcat(addedCommas,token_part2);
			printf(" (%12.10s) |",addedCommas);
		}else{
			snprintf(tempChar,50,"%lf",temp);
			strcpy(token_part1,strtok(tempChar,"."));
			strcpy(token_part2,strtok(NULL,"."));
			token_part2[2]='\0';
			strcpy(addedCommas,addCommas(token_part1));
			strcat(addedCommas,".");
			strcat(addedCommas,token_part2);
			printf(" %13.10s  |",addedCommas);
		}		
	}
	printf("\n+-----------------+--------------------------+----------------+----------------+");
	printf("\n");
}

static
void BubbleForward(My402List *pList, My402ListElem **pp_elem1, My402ListElem **pp_elem2)

{
    My402ListElem *elem1=(*pp_elem1), *elem2=(*pp_elem2);
    void *obj1=elem1->obj, *obj2=elem2->obj;
    My402ListElem *elem1prev=My402ListPrev(pList, elem1);
    My402ListElem *elem2next=My402ListNext(pList, elem2);

    My402ListUnlink(pList, elem1);
    My402ListUnlink(pList, elem2);
    if (elem1prev == NULL) {
        (void)My402ListPrepend(pList, obj2);
        *pp_elem1 = My402ListFirst(pList);
    } else {
        (void)My402ListInsertAfter(pList, obj2, elem1prev);
        *pp_elem1 = My402ListNext(pList, elem1prev);
    }
    if (elem2next == NULL) {
        (void)My402ListAppend(pList, obj1);
        *pp_elem2 = My402ListLast(pList);
    } else {
        (void)My402ListInsertBefore(pList, obj1, elem2next);
        *pp_elem2 = My402ListPrev(pList, elem2next);
    }
}

void sortList(My402List *node){
	int num_items = My402ListLength(node);
    My402ListElem *elem=NULL;
    struct nodeVariables *ptr,*ptr1= NULL;
    int i=0;

    for (i=0; i < num_items; i++) {
        int j=0, something_swapped=FALSE;
        My402ListElem *next_elem=NULL;

        for (elem=My402ListFirst(node), j=0; j < num_items-i-1; elem=next_elem, j++) {
        	ptr = elem->obj;
            int cur_val=(int)ptr->transTime, next_val=0;
            next_elem=My402ListNext(node, elem);
            ptr1= next_elem->obj;
            next_val = (int)ptr1->transTime;
            if(cur_val == next_val){
            	printf("\nMalformed Input, Duplicate TimeStamp Detected !\n");
            	exit(1);
            }
            if (cur_val > next_val) {
                BubbleForward(node, &elem, &next_elem);
                something_swapped = TRUE;
            }
        }
        if (!something_swapped) break;
    }
}


double doTransaction (My402ListElem *elem)
{
	struct nodeVariables *ptr = NULL;
		ptr = elem->obj;
		if(atof(ptr->transAmt) > 10000000)
		{
			return 10000023.00;
		}
		if(strcmp(ptr->symbol,"+")==0){
			balance = balance + atof(ptr->transAmt);
		}
		else if(strcmp(ptr->symbol,"-")==0){
			balance = balance - atof(ptr->transAmt);			
		}
	return balance;
}

char* addCommas(char *inChar){
	    int rev_str = strlen(inChar), commas;
	    if (rev_str % 3 == 0){
	        commas = (rev_str / 3) - 1;
	    }
	    else{
	        int mod = rev_str % 3;
	        commas = ( rev_str - mod ) / 3;
	    }
	    rev_str = commas + strlen(inChar);
	    char * new_str = (char *)malloc(rev_str);
	    int i = strlen(inChar) - 1;
	    for (; i >= 0; i--){
	        if (((strlen(inChar) - i) % 3 == 0) && i != 0){
	            new_str[i + commas] = inChar[i];
	            new_str[i + --commas] = ',';
	        }
	        else{
	            new_str[i + commas] = inChar[i];
	        }
	    }
	    new_str[rev_str] = '\0';
	    return new_str;
	    }

void transform (My402List *node){
	My402ListElem *elem = NULL;
	struct nodeVariables *ptr = NULL;
	char token_part1[10];
	char token_part2[10];
	char addedCommas[30];
		for (elem=My402ListFirst(node);elem!=NULL;elem=My402ListNext(node,elem)){
			ptr= elem->obj;
			if(atof(ptr->transAmt) > 10000000)
			{
				strcpy(ptr->printableAmt,"?,???,???.??");
			}else{
			strcpy(ptr->printableAmt,ptr->transAmt);
			strcpy(token_part1,strtok(ptr->printableAmt,"."));
			strcpy(token_part2,strtok(NULL,"."));
			strcpy(addedCommas,addCommas(token_part1));
			strcat(addedCommas,".");
			strcat(addedCommas,token_part2);
			strcpy(ptr->printableAmt,addedCommas);
			}
		}
	}

int main(int argc,char **argv){
	if(argc<2){
		printf("\nMalformed Command. usage: warmup1 sort [tfile]\n");
		exit(1);
	}
	if(!(strcmp(argv[1],"sort")==0))
	{
		printf("\nMalformed Input. usage: warmup1 sort [tfile]\n");
		exit(1);
	}
	readFile(argc,argv[2]);
	sortList(&pList);
	transform(&pList);
	printList(&pList);
	return 0;
	
}

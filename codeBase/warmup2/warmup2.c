	#include <stdio.h>
	#include <string.h>
	#include <math.h>
	#include <stdlib.h>
	#include <pthread.h>
	#include <time.h>
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/time.h>
	#include <signal.h>
	#include "my402list.h"


	double lambda=1;
	double mu_value = 0.35;
	double r_value =1.5;
	long int bucketDepth = 10;
	long int reqTokensToPass = 3;
	long int num = 20;
	
	sigset_t set;

	pthread_mutex_t mutex_Var;

	static int readFile_flag = 0;

	struct packetInfo{
		int packetNo;
		double sysArrTime;
		long int reqTokens;
		double Q1ArrTime;
		double Q1DepTime;
		double Q2ArrTime;
		double Q2DepTime;
		double serviceStartTime;
		double ServiceEndTime;
		double mu_file;
	};

	struct filePacketInfo{
		int file_packetNo;
		double file_interval_time;
		double file_service_time;
		long int file_reqTokenToPass;
	};

	pthread_t packetArrivalThread;
	pthread_t tokenBucketThread;
	pthread_t server1;
	pthread_t server2;
	pthread_t ctrlThread;
	pthread_cond_t guard = PTHREAD_COND_INITIALIZER;

	My402List pList1;
	My402List Q1List,Q2List;

	struct filePacketInfo *filePackPtr;
	double simu_startTime,simu_endTime;

	double total_intervalTime=0;
	double total_serviceTime_S1=0;
	double total_serviceTime_S2=0;
	double total_serviceTime=0;
	double total_Q1Time=0;
	double total_Q2Time=0;
	double total_S1Time=0;
	double total_S2Time=0;
	double total_time_spent=0;
	double total_time_spent_sq=0;
	double inter_start,inter_end,service_start_S1,service_end_S1,service_start_S2,service_end_S2=0;
	int total_genPackets=0;
	int packetCount_S1,packetCount_S2=0;
	int total_servicePackets=0;

	int q1_to_q2,q2_to_out=0;
	int p_flag,t_flag,s1_flag,s2_flag=1;
	int bucket_token_count =0;
	int droppedToken=0;
	int flag_server=1;
	int total_token_gen=0;
	int droppedPackets=0;
	char *fileName;

	double getTimeStamp(){
		struct timeval arg;
		gettimeofday(&arg, NULL);
		double returnTime;
		returnTime = ((double)(((double)arg.tv_usec)/1000 + arg.tv_sec*1000));
		return returnTime;
	}

	void readFile(int inArg,char * inFile){
		My402ListInit(&pList1);
		char line[1026];
		double interval_time=0;
		double service_time=0;
		FILE *file = NULL;
		int j;
		char tabChar_count[1026];
		int tab_count=0;
		int i=1;
		int k=0;
		struct stat stat_var;
		lstat(inFile, &stat_var);
		if(S_ISDIR(stat_var.st_mode)){
			printf("\nThe given input is a directory , Malformed Input\n");
	        exit(1);
	    }
	    if(inArg>1){
		    if(fopen(inFile,"r")){
				file=fopen(inFile,"r");
			}else if(!(access (inFile, F_OK)==0)){
				printf("\nThe file dosent exist or access denied !!, Malformed Command\n");
				exit(1);
			}
			if(!(access(inFile, R_OK) == 0)){
			    printf("\nPermission Denied, Malformed Command !\n");
			    exit(1);
			}	
		}else{
			printf("\nInvalid Argument, Malformed Command !\n");
			exit(1);
		}
		while(fgets(line, sizeof(line),file)){
		tab_count=0;
		filePackPtr = (struct filePacketInfo*)malloc(sizeof(struct filePacketInfo));
		strcpy(tabChar_count,line);
		for(j=0;tabChar_count[j] !='\0';j++){
			if(tabChar_count[j]=='\t'){
				tab_count++;
			}
		}
		if(tab_count>2){
			printf("\nMalformed Input, please check the number of spaces of tabs\n");
			exit(1);
		}
		if(k==0){
			num = atol(line);
			k++;
			continue;
		}else{
			sscanf(line,"%lf\t%ld\t%lf\n",&interval_time,&reqTokensToPass,&service_time);
			}
		if(num<=0 || num > 2147483647){
			printf("\nInvalid file format. Please check the value of num to arrive\n");
			exit(1);
		}
		if(service_time <0){
			printf("\nThe value of mu cannot be negative, please enter a positive value for mu\n");
			exit(1);
		}
		if(reqTokensToPass<=0 || reqTokensToPass > 2147483647){
			printf("\nPlease enter a valid P value\n");
			exit(1);
		}
		if(interval_time<0){
			printf("\nThe value of lambda cannot be negative, please enter a positive value for lambda\n");
			exit(1);
		}
		filePackPtr->file_packetNo=i;
		filePackPtr->file_reqTokenToPass=reqTokensToPass;
		filePackPtr->file_service_time=service_time;
		filePackPtr->file_interval_time=interval_time;
		My402ListAppend(&pList1,filePackPtr);
		i++;
		}
		fclose(file);
	}

	void *packetArrival(){
	 	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
	 	static int i=1,j=1;
	 	double timeLapse=0;
	 	double tempVar=0;
		double sleep_start,sleep_end=0;
		struct packetInfo *packet_obj;
		int temp_tok = 0;
		double sleepTime=0;
		My402ListElem *elem,*Q1_elem=NULL;
		if(readFile_flag){
			struct filePacketInfo *ptr=NULL;
			for (elem=My402ListFirst(&pList1);elem!=NULL;elem=My402ListNext(&pList1,elem)){
					j++;
					ptr=elem->obj;
					lambda = ptr->file_interval_time;
					inter_start = getTimeStamp()-simu_startTime;
					if((sleepTime-timeLapse)>lambda){
						if((lambda-timeLapse)<0){
							tempVar = -1*(lambda-timeLapse);
							usleep(tempVar*1000);
						}else{
						usleep((lambda-timeLapse)*1000);
						}
					}else if(timeLapse>=lambda){
						usleep(0);
					}else{
						usleep(lambda*1000);
					}
					inter_end = getTimeStamp()-simu_startTime;
					sleepTime = (inter_end-inter_start);
					total_intervalTime = total_intervalTime + (sleepTime);
					sleep_start = getTimeStamp()-simu_startTime;
					packet_obj=(struct packetInfo*)malloc(sizeof(struct packetInfo));
					packet_obj->packetNo = ptr->file_packetNo;
					packet_obj->reqTokens = ptr->file_reqTokenToPass;
					packet_obj->mu_file = ptr->file_service_time;
					packet_obj->sysArrTime = (getTimeStamp()-simu_startTime);
					total_genPackets++;
					printf("\n%012.3fms: p%d arrives, needs %ld tokens, inter-arrival time = %.3lfms",(getTimeStamp()-simu_startTime),packet_obj->packetNo,packet_obj->reqTokens,(inter_end-inter_start));
					packet_obj->Q1ArrTime = (getTimeStamp()-simu_startTime);
					if(packet_obj->reqTokens>bucketDepth){
						droppedPackets++;
						printf("\n%012.3fms: p%d arrives, needs %ld tokens, inter-arrival time = %.3lfms, dropped",(getTimeStamp()-simu_startTime),packet_obj->packetNo,packet_obj->reqTokens,(inter_end-inter_start));
						continue;
					}
					pthread_mutex_lock(&mutex_Var);
					printf("\n%012.3lfms: p%d enters Q1",(getTimeStamp()-simu_startTime),packet_obj->packetNo);
					My402ListAppend(&Q1List,packet_obj);
					pthread_mutex_unlock(&mutex_Var);
					Q1_elem = (My402ListElem *)malloc(sizeof(My402ListElem));
					pthread_mutex_lock(&mutex_Var);
					if(My402ListLength(&Q1List)!=0){
						Q1_elem = My402ListFirst(&Q1List);
						packet_obj = Q1_elem->obj;
						temp_tok = packet_obj->reqTokens;
						if(packet_obj->reqTokens<=bucket_token_count){
							bucket_token_count=bucket_token_count-temp_tok;
							packet_obj->Q1DepTime = (getTimeStamp()-simu_startTime);
							total_Q1Time = total_Q1Time + (packet_obj->Q1DepTime-packet_obj->Q1ArrTime);
							printf("\n%012.3lfms: p%d leaves Q1, time in Q1 = %.3lfms, token bucket now has %d tokens",(getTimeStamp()-simu_startTime),packet_obj->packetNo,(packet_obj->Q1DepTime-packet_obj->Q1ArrTime),bucket_token_count);
							My402ListUnlink(&Q1List,Q1_elem);
							packet_obj->Q2ArrTime = (getTimeStamp()-simu_startTime);
							if(My402ListAppend(&Q2List,packet_obj)){
									printf("\n%012.3lfms: p%d enters Q2",(getTimeStamp()-simu_startTime),packet_obj->packetNo);
									pthread_cond_broadcast(&guard);
								q1_to_q2++;
							}
						}
					}
				sleep_end = getTimeStamp()-simu_startTime;
				timeLapse = sleep_end-sleep_start;
				pthread_mutex_unlock(&mutex_Var);
			}
			if(((j-1)+droppedPackets)==num){
				p_flag =1;
				pthread_mutex_unlock(&mutex_Var);
				pthread_cond_broadcast(&guard);
				pthread_exit(0);
			}
		}else{
			while(1){	
				inter_start = getTimeStamp()-simu_startTime;
				if((sleepTime-timeLapse)>((1/lambda)*1000)){
					if((((1/lambda)*1000)-timeLapse)<0){
							tempVar = -1*(((1/lambda)*1000)-timeLapse);
							usleep(tempVar*1000);
						}else{
							usleep((((1/lambda)*1000)-timeLapse)*1000);
						}
				}else if(timeLapse>=((1/lambda)*1000)){
					usleep(0);
				}else{
					usleep((1/lambda)*1000000);
				}
				if(((i-1)==num)){
					p_flag=1;
					pthread_mutex_unlock(&mutex_Var);
					pthread_cond_broadcast(&guard);
					pthread_exit(0);
				}
				inter_end = getTimeStamp()-simu_startTime;
				sleepTime = (inter_end-inter_start);
				total_intervalTime = total_intervalTime + (sleepTime);
				sleep_start = getTimeStamp()-simu_startTime;
				packet_obj=(struct packetInfo*)malloc(sizeof(struct packetInfo));
				packet_obj->packetNo = i++;
				packet_obj->reqTokens = reqTokensToPass;
				packet_obj->sysArrTime = (getTimeStamp()-simu_startTime);
				total_genPackets++;
				printf("\n%012.3lfms: p%d arrives, needs %ld tokens, inter-arrival time = %.3lfms",(getTimeStamp()-simu_startTime),packet_obj->packetNo,packet_obj->reqTokens,(inter_end-inter_start));
				packet_obj->Q1ArrTime = (getTimeStamp()-simu_startTime);
				if(packet_obj->reqTokens>bucketDepth){
					droppedPackets++;
					printf("\n%012.3lfms: p%d arrives, needs %ld tokens, inter-arrival time = %.3lfms, dropped",(getTimeStamp()-simu_startTime),packet_obj->packetNo,packet_obj->reqTokens,(inter_end-inter_start));
					continue;
				}
				pthread_mutex_lock(&mutex_Var);
				printf("\n%012.3lfms: p%d enters Q1",(getTimeStamp()-simu_startTime),packet_obj->packetNo);
				My402ListAppend(&Q1List,packet_obj);
				pthread_mutex_unlock(&mutex_Var);
				Q1_elem = (My402ListElem *)malloc(sizeof(My402ListElem));
				pthread_mutex_lock(&mutex_Var);
				if(My402ListLength(&Q1List)!=0){
					Q1_elem = My402ListFirst(&Q1List);
					packet_obj = Q1_elem->obj;
					temp_tok = packet_obj->reqTokens;
					if(packet_obj->reqTokens<=bucket_token_count){
						bucket_token_count=bucket_token_count-temp_tok;
						packet_obj->Q1DepTime = (getTimeStamp()-simu_startTime);
						total_Q1Time = total_Q1Time + (packet_obj->Q1DepTime-packet_obj->Q1ArrTime);
						printf("\n%012.3lfms: p%d leaves Q1, time in Q1 = %.3lfms, token bucket now has %d tokens",(getTimeStamp()-simu_startTime),packet_obj->packetNo,(packet_obj->Q1DepTime-packet_obj->Q1ArrTime),bucket_token_count);
						My402ListUnlink(&Q1List,Q1_elem);
						packet_obj->Q2ArrTime = (getTimeStamp()-simu_startTime);
						if(My402ListAppend(&Q2List,packet_obj)){
								printf("\n%012.3lfms: p%d enters Q2",(getTimeStamp()-simu_startTime),packet_obj->packetNo);
								pthread_cond_broadcast(&guard);
							q1_to_q2++;
						}
					}
				}
			sleep_end = getTimeStamp()-simu_startTime;
			timeLapse = sleep_end-sleep_start;
			pthread_mutex_unlock(&mutex_Var);
			//i++;
			}
		}
		return NULL;
	}

	void *tokenBucket(){
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
		int temp_tok_2 =0;
		double tempVar=0;
		double timeLapse2,tok_start,tok_end,sleepTime2=0;
		int i=1;
		struct packetInfo *packet_obj_2;
		My402ListElem *Q1_elem_2=NULL;
		double sleep_start_2,sleep_end_2=0;
			while(1){
				tok_start = getTimeStamp()-simu_startTime;
				if((sleepTime2-timeLapse2)>((1/r_value)*1000)){
					if((((1/r_value)*1000)-timeLapse2)<0){
						tempVar =-1*(((1/r_value)*1000)-timeLapse2);
						usleep(tempVar*1000);
					}else{
					usleep((((1/r_value)*1000)-timeLapse2)*1000);
					}
				}else if(timeLapse2>=((1/r_value)*1000)){
					usleep(0);
				}else{
					usleep(((1/r_value)*1000000));
				}
				sleep_start_2 = getTimeStamp()-simu_startTime;
				tok_end = getTimeStamp()-simu_startTime;
				sleepTime2 = (tok_end-tok_start);
				pthread_mutex_lock(&mutex_Var);
				if(total_genPackets != num){
					if(bucket_token_count<bucketDepth){
						bucket_token_count=bucket_token_count+1;
						printf("\n%012.3lfms: token t%d arrives, token bucket now has %d tokens",(getTimeStamp()-simu_startTime),i,bucket_token_count);
					}else{
						printf("\n%012.3lfms: token t%d arrives, dropped",(getTimeStamp()-simu_startTime),i);
						droppedToken++;
					}
					i=i+1;
				}
				total_token_gen = (i-1);
				pthread_mutex_unlock(&mutex_Var);
				packet_obj_2=(struct packetInfo*)malloc(sizeof(struct packetInfo));
				Q1_elem_2 = (My402ListElem *)malloc(sizeof(My402ListElem));
				pthread_mutex_lock(&mutex_Var);
				if(My402ListLength(&Q1List)!=0){
					Q1_elem_2 = My402ListFirst(&Q1List);
					packet_obj_2 = Q1_elem_2->obj;
					temp_tok_2 = packet_obj_2->reqTokens;
					if(packet_obj_2->reqTokens<=bucket_token_count){
						bucket_token_count=bucket_token_count-temp_tok_2;
						packet_obj_2->Q1DepTime = (getTimeStamp()-simu_startTime);
						total_Q1Time = total_Q1Time + (packet_obj_2->Q1DepTime-packet_obj_2->Q1ArrTime);
						printf("\n%012.3lfms: p%d leaves Q1, time in Q1 = %.3lfms, token bucket now has %d tokens",(getTimeStamp()-simu_startTime),packet_obj_2->packetNo,(packet_obj_2->Q1DepTime-packet_obj_2->Q1ArrTime),bucket_token_count);
						My402ListUnlink(&Q1List,Q1_elem_2);
						packet_obj_2->Q2ArrTime = (getTimeStamp()-simu_startTime);
						if(My402ListAppend(&Q2List,packet_obj_2))
						{
							printf("\n%012.3lfms: p%d enters Q2",(getTimeStamp()-simu_startTime),packet_obj_2->packetNo);
							pthread_cond_broadcast(&guard);
							q1_to_q2++;
						}
						
					}
				}else if(My402ListEmpty(&Q1List) && ((q1_to_q2+droppedPackets)==num)){
					pthread_mutex_unlock(&mutex_Var);
					t_flag =1;
					pthread_cond_broadcast(&guard);
					pthread_exit(0);
				}
			sleep_end_2 = getTimeStamp()-simu_startTime;
			timeLapse2 = sleep_end_2-sleep_start_2;
			pthread_mutex_unlock(&mutex_Var);
			}	
		return NULL;
	}

	void *server1Impl(){
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
		struct packetInfo *packet_obj_3;
		double mu_file_temp=0;
		My402ListElem *Q2_elem_2=NULL;
			Q2_elem_2 = (My402ListElem *)malloc(sizeof(My402ListElem));
			packet_obj_3=(struct packetInfo*)malloc(sizeof(struct packetInfo));
			while(1){
			pthread_mutex_lock(&mutex_Var);
			if(!flag_server){
						if(!s2_flag){
							pthread_cancel(server2);
						}
						pthread_mutex_unlock(&mutex_Var);
						pthread_exit(0);
					}
			while(My402ListEmpty(&Q2List)){
				if(!flag_server){
						if(!s2_flag){
							pthread_cancel(server2);
						}
						pthread_mutex_unlock(&mutex_Var);
						pthread_exit(0);
					}
				if((q2_to_out+droppedPackets) == num){
					pthread_mutex_unlock(&mutex_Var);
					if(!p_flag){
						pthread_cancel(packetArrivalThread);
					}if(!t_flag){
						pthread_cancel(tokenBucketThread);
					}if(!s2_flag){
						pthread_cancel(server2);
					}
					pthread_exit(0);
				}else{
					s1_flag =0;
					pthread_cond_wait(&guard, &mutex_Var);
					s1_flag =1;
				}
			}
			if(My402ListLength(&Q2List)!=0){
					Q2_elem_2 = My402ListFirst(&Q2List);
					packet_obj_3 = Q2_elem_2->obj;
					packet_obj_3->serviceStartTime = (getTimeStamp()-simu_startTime);
					packet_obj_3->Q2DepTime = (getTimeStamp()-simu_startTime);
					mu_file_temp = packet_obj_3->mu_file;
					total_Q2Time = total_Q2Time +(packet_obj_3->Q2DepTime-packet_obj_3->Q2ArrTime);
					printf("\n%012.3lfms: p%d leaves Q2, time in Q2 = %.3lfms",(getTimeStamp()-simu_startTime),packet_obj_3->packetNo,(packet_obj_3->Q2DepTime - packet_obj_3->Q2ArrTime));
					packetCount_S1++;
					My402ListUnlink(&Q2List,Q2_elem_2);
					if(readFile_flag){
						printf("\n%012.3lfms: p%d begins service at S1, requesting %.3lfms of service",(getTimeStamp()-simu_startTime),packet_obj_3->packetNo,mu_file_temp);
					}else{
						printf("\n%012.3lfms: p%d begins service at S1, requesting %.3lfms of service",(getTimeStamp()-simu_startTime),packet_obj_3->packetNo,((1/mu_value)*1000));
					}
					if(readFile_flag){
						pthread_mutex_unlock(&mutex_Var);
						usleep(mu_file_temp*1000);
					}else{
						pthread_mutex_unlock(&mutex_Var);
						usleep(((1/mu_value)*1000000));
					}
					pthread_mutex_lock(&mutex_Var);
					packet_obj_3->ServiceEndTime = (getTimeStamp()-simu_startTime);
					total_S1Time = total_S1Time + (packet_obj_3->ServiceEndTime-packet_obj_3->serviceStartTime);
					total_time_spent = (total_time_spent + ((getTimeStamp()-simu_startTime)-packet_obj_3->sysArrTime));
					total_time_spent_sq=total_time_spent_sq + ((getTimeStamp()-simu_startTime)-packet_obj_3->sysArrTime)*((getTimeStamp()-simu_startTime)-packet_obj_3->sysArrTime);
					printf("\n%012.3lfms: p%d departs from S1, service time = %.3lfms, time in system = %.3lfms",(getTimeStamp()-simu_startTime),packet_obj_3->packetNo,(packet_obj_3->ServiceEndTime-packet_obj_3->serviceStartTime),((getTimeStamp()-simu_startTime)-packet_obj_3->sysArrTime));
					q2_to_out++;
					//serverdone1=1;
				}
			pthread_mutex_unlock(&mutex_Var);
			}
		return NULL;
	}
	void *server2Impl(){
		pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
		struct packetInfo *packet_obj_4;
		double mu_file_temp2=0;
		My402ListElem *Q2_elem_3=NULL;
			Q2_elem_3 = (My402ListElem *)malloc(sizeof(My402ListElem));
			packet_obj_4=(struct packetInfo*)malloc(sizeof(struct packetInfo));
			while(1){
			pthread_mutex_lock(&mutex_Var);
			if(!flag_server){
					if(!s1_flag){
						pthread_cancel(server1);
					}
					pthread_mutex_unlock(&mutex_Var);
					pthread_exit(0);
				}
			while(My402ListEmpty(&Q2List)){
				if(!flag_server){
					if(!s1_flag){
						pthread_cancel(server1);
					}
					pthread_mutex_unlock(&mutex_Var);
					pthread_exit(0);
				}
				if((q2_to_out+droppedPackets) == num){
					pthread_mutex_unlock(&mutex_Var);
					if(!p_flag){
						pthread_cancel(packetArrivalThread);
					}if(!t_flag){
						pthread_cancel(tokenBucketThread);
					}if(!s1_flag){
						pthread_cancel(server1);
					}
					pthread_exit(0);
				}else{
					s2_flag=0;
					pthread_cond_wait(&guard, &mutex_Var);
					s2_flag=1;
				}
			}
			if(My402ListLength(&Q2List)!=0){
				Q2_elem_3 = My402ListFirst(&Q2List);
				packet_obj_4 = Q2_elem_3->obj;
				packet_obj_4->Q2DepTime = (getTimeStamp()-simu_startTime);
				packet_obj_4->serviceStartTime = (getTimeStamp()-simu_startTime);
				mu_file_temp2 = packet_obj_4->mu_file;
				total_Q2Time = total_Q2Time +(packet_obj_4->Q2DepTime-packet_obj_4->Q2ArrTime);
				printf("\n%012.3lfms: p%d leaves Q2, time in Q2 = %.3lfms",(getTimeStamp()-simu_startTime),packet_obj_4->packetNo,(packet_obj_4->Q2DepTime - packet_obj_4->Q2ArrTime));
				packetCount_S2++;
				My402ListUnlink(&Q2List,Q2_elem_3);
				if(readFile_flag){
					printf("\n%012.3lfms: p%d begins service at S2, requesting %.3lfms of service",(getTimeStamp()-simu_startTime),packet_obj_4->packetNo,mu_file_temp2);
				}else{
					printf("\n%012.3lfms: p%d begins service at S2, requesting %.3lfms of service",(getTimeStamp()-simu_startTime),packet_obj_4->packetNo,((1/mu_value)*1000));
				}
				if(readFile_flag){
					pthread_mutex_unlock(&mutex_Var);
					usleep(mu_file_temp2*1000);
				}else{
					pthread_mutex_unlock(&mutex_Var);
					usleep(((1/mu_value)*1000000));
				}
				pthread_mutex_lock(&mutex_Var);
				packet_obj_4->ServiceEndTime = (getTimeStamp()-simu_startTime);
				total_S2Time = total_S2Time + (packet_obj_4->ServiceEndTime-packet_obj_4->serviceStartTime);
				total_time_spent = (total_time_spent + ((getTimeStamp()-simu_startTime)-packet_obj_4->sysArrTime));
				total_time_spent_sq=total_time_spent_sq+((getTimeStamp()-simu_startTime)-packet_obj_4->sysArrTime)*((getTimeStamp()-simu_startTime)-packet_obj_4->sysArrTime);
				printf("\n%012.3lfms: p%d departs from S2, service time = %.3lfms, time in system = %.3lfms",(getTimeStamp()-simu_startTime),packet_obj_4->packetNo,(packet_obj_4->ServiceEndTime-packet_obj_4->serviceStartTime),((getTimeStamp()-simu_startTime)-packet_obj_4->sysArrTime));
				q2_to_out++;
			}
			pthread_mutex_unlock(&mutex_Var);	
			}
		return NULL;
	}

	void statistics(){
		double avg_packet_inter_arrival_time=0;
		double avg_packet_service_time=0;
		double avg_no_of_packets_in_Q1=0;
		double avg_no_of_packets_in_Q2=0;
		double avg_no_of_packets_in_S2=0;
		double avg_no_of_packets_in_S1=0;
		double avg_time_packet=0;
		double sd_value=0;
		double var_temp=0;
		double var_value=0;
		double token_prob=0;
		double packet_prob=0;
		long int total_packets_serviced=0;

		total_packets_serviced = packetCount_S2+packetCount_S1;
		avg_packet_inter_arrival_time = (((double)(total_intervalTime/(double)total_genPackets))/1000);
		avg_packet_service_time = ((total_S2Time + total_S1Time)/(packetCount_S2+packetCount_S1)/1000);
		avg_no_of_packets_in_Q1 = (total_Q1Time/(simu_endTime-simu_startTime));
		avg_no_of_packets_in_Q2 = (total_Q2Time/(simu_endTime-simu_startTime));
		avg_no_of_packets_in_S1 = (total_S1Time/(simu_endTime-simu_startTime));
		avg_no_of_packets_in_S2 = (total_S2Time/(simu_endTime-simu_startTime));
		avg_time_packet = (total_time_spent/total_packets_serviced);
		var_temp = (total_time_spent_sq/total_packets_serviced);
		var_value = var_temp-(avg_time_packet*avg_time_packet);
		sd_value = sqrt(var_value)/1000;
		token_prob = ((double)droppedToken/(double)total_token_gen);
		packet_prob = ((double)droppedPackets/(double)total_genPackets);

		printf("\nStatistics:");
		printf("\n");
		if(total_genPackets==0){
			printf("\n\taverage packet inter-arrival time = N/A (No packets were generated)");
		}else{
			printf("\n\taverage packet inter-arrival time = %.6g",avg_packet_inter_arrival_time);
		}
		if(packetCount_S1 ==0 && packetCount_S2 == 0){
			printf("\n\taverage packet service time = N/A (No packets were serviced)");	
		}else{
			printf("\n\taverage packet service time = %.6g",avg_packet_service_time);
		}
		printf("\n");
		if(total_genPackets==0){
			printf("\n\tavergae number of packets in Q1 = N/A (No packets were generated)");
		}else{
			printf("\n\taverage number of packets in Q1 = %.6g",avg_no_of_packets_in_Q1);
		}
		if(total_genPackets==0 || q1_to_q2 ==0){
			printf("\n\taverage number of packets in Q2 = N/A (No packets were generated or no packets made to Q2)");
		}else{
			printf("\n\taverage number of packets in Q2 = %.6g",avg_no_of_packets_in_Q2);
		}
		if(packetCount_S1 == 0){
			printf("\n\taverage number of packets in S1 = N/A (No packets were serviced in S1)");
		}else{
			printf("\n\taverage number of packets in S1 = %.6g",avg_no_of_packets_in_S1);	
		}if(packetCount_S2 == 0){
			printf("\n\taverage number of packets in S2 = N/A (No packets were serviced in S2)");
		}else{
			printf("\n\taverage number of packets in S2 = %.6g",avg_no_of_packets_in_S2);
		}
		printf("\n");
		if(total_genPackets==0){
			printf("\n\taverage time a packet spent in system = N/A (No packets were generated)");
		}else if((packetCount_S1 + packetCount_S2)==0){
			printf("\n\taverage time a packet spent in system = N/A (No packets were serviced)");
			printf("\n\tstandard deviation for time spent in system = N/A (No packets were serviced)");
		}else if((total_genPackets+(droppedPackets))==1){
			printf("\n\tstandard deviation for time spent in system = 0");
		}else{
			printf("\n\taverage time a packet spent in system = %.6g",avg_time_packet/1000);
			printf("\n\tstandard deviation for time spent in system = %.6g",sd_value);
		}
		printf("\n");
		if(total_token_gen==0){
			printf("\n\ttoken drop probability = N/A (No tokens were generated");
		}else{
			printf("\n\ttoken drop probability = %.6g",token_prob);
		}
		if(total_genPackets==0){
			printf("\n\tpacket drop probability = N/A (No packets were generated\n");
		}else{
			printf("\n\tpacket drop probability = %.6g\n",packet_prob);
		}
	}

	void *signalHandler(){
		int sig=0;
		sigwait(&set,&sig);
		simu_endTime = getTimeStamp();
		pthread_cancel(packetArrivalThread);
		pthread_cancel(tokenBucketThread);
		flag_server=0;
		struct packetInfo *packet_obj;
		My402ListElem *elem=NULL;
		pthread_mutex_lock(&mutex_Var);
		if(!My402ListEmpty(&Q1List)){
			q2_to_out=q2_to_out+My402ListLength(&Q1List);
			while(My402ListLength(&Q1List)){
				elem=My402ListFirst(&Q1List);
				packet_obj = (struct packetInfo*)malloc(sizeof(struct packetInfo));
				packet_obj = elem->obj;	
				printf("\n%012.3lfms: p%d dropped from Q1",(getTimeStamp()-simu_startTime),packet_obj->packetNo);
				My402ListUnlink(&Q1List,elem);
			}
		}	
		if(!My402ListEmpty(&Q2List)){
			q2_to_out=q2_to_out+My402ListLength(&Q2List);
			while(My402ListLength(&Q2List)){
				elem=My402ListFirst(&Q2List);
				packet_obj = (struct packetInfo*)malloc(sizeof(struct packetInfo));
				packet_obj = elem->obj;
				printf("\n%012.3lfms: p%d dropped from Q2",(getTimeStamp()-simu_startTime),packet_obj->packetNo);
				My402ListUnlink(&Q2List,elem);
			}
		}
		pthread_cond_broadcast(&guard);
		pthread_mutex_unlock(&mutex_Var);
		
		pthread_exit(0);
		return NULL;
	}

	int main(int argc, char **argv){
		int i;
		int lf=1;
		int rf=1;
		int pf=1;
		int tf=1;
		int nf=1;
		int bf=1;
		int muf=1;
		My402ListInit(&Q1List);
		My402ListInit(&Q2List);
		sigemptyset(&set);
		sigaddset(&set,SIGINT);
		sigprocmask(SIG_BLOCK,&set,0);
		if(argc<1){
			printf("\nMalformed Command ! usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
			exit(1);
		}
		if(argc>1){
			if(argc%2 == 0){
				printf("\nMalformed Command ! usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}
		for(i=0;i<argc;i++){
			if(strcmp(argv[i],"-lambda")==0 || strcmp(argv[i],"[-lambda")==0){
				lf=0;
				if(argv[i+1][strlen(argv[i+1])-1]==']'){
					argv[i+1][strlen(argv[i+1])-1]='\0';
					lambda = atof(argv[i+1]);
					if((1/lambda)>10){
						lambda = 0.1;
					}
					else if(lambda<0){
						printf("\nThe value of lambda cannot be negative, please enter a positive value for lambda\n");
						exit(1);
					}
				}else{
					lambda = atof(argv[i+1]);
					if((1/lambda)>10){
						lambda = 0.1;
					}
					else if(lambda<0){
						printf("\nThe value of lambda cannot be negative, please enter a positive value for lambda\n");
						exit(1);
					}
				}
			} 
			if(strcmp(argv[i],"-mu")==0 || strcmp(argv[i],"[-mu")==0){
				muf=0;
				if(argv[i+1][strlen(argv[i+1])-1]==']'){
					argv[i+1][strlen(argv[i+1])-1]='\0';
					mu_value = atof(argv[i+1]);
					if((1/mu_value)>10){
						mu_value = 0.1;
					}
					else if(mu_value<0){
						printf("\nThe value of mu cannot be negative, please enter a positive value for mu\n");
						exit(1);
					}
				}else{
					mu_value = atof(argv[i+1]);
					if((1/mu_value)>10){
						mu_value = 0.1;
					}
					else if(mu_value<0){
						printf("\nThe value of mu cannot be negative, please enter a positive value for mu\n");
						exit(1);
					}
				}
			}
			if(strcmp(argv[i],"-r")==0 || strcmp(argv[i],"[-r")==0){
					rf=0;
				if(argv[i+1][strlen(argv[i+1])-1]==']'){
					argv[i+1][strlen(argv[i+1])-1]='\0';
					r_value = atof(argv[i+1]);
					if((1/r_value)>10){
						r_value = 0.1;
					}
					else if(r_value<0){
						printf("\nThe value of r cannot be negative, please enter a positive value for r\n");
						exit(1);
					}
				}else{
					r_value = atof(argv[i+1]);
					if((1/r_value)>10){
						r_value = 0.1;
					}
					else if(r_value<0){
						printf("\nThe value of r cannot be negative, please enter a positive value for r\n");
						exit(1);
					}
				}
			}
			if(strcmp(argv[i],"-B")==0 || strcmp(argv[i],"[-B")==0){
				 	bf=0;
				if(argv[i+1][strlen(argv[i+1])-1]==']'){
					argv[i+1][strlen(argv[i+1])-1]='\0';
					bucketDepth = atol(argv[i+1]);
					if(bucketDepth <0 || bucketDepth > 2147483647)
					{
						printf("\nPlease enter a valid B value\n");
						exit(1);
					}
				}else{
					bucketDepth = atol(argv[i+1]);
					if(bucketDepth<0 || bucketDepth > 2147483647)
					{
						printf("\nPlease enter a valid B value\n");
						exit(1);
					}
				}
			}
			if(strcmp(argv[i],"-P")==0 || strcmp(argv[i],"[-P")==0){
			 	pf=0;
				if(argv[i+1][strlen(argv[i+1])-1]==']'){
					argv[i+1][strlen(argv[i+1])-1]='\0';
					reqTokensToPass = atol(argv[i+1]);
					if(reqTokensToPass < 0 || reqTokensToPass > 2147483647)
					{
						printf("\nPlease enter a valid P value\n");
						exit(1);
					}	
				}else{
					reqTokensToPass = atol(argv[i+1]);
					if(reqTokensToPass < 0 || reqTokensToPass > 2147483647)
					{
						printf("\nPlease enter a valid P value\n");
						exit(1);
					}
				}
			}
			if(strcmp(argv[i],"-n")==0 || strcmp(argv[i],"[-n")==0){
			 	nf=0;
				if(argv[i+1][strlen(argv[i+1])-1]==']'){
					argv[i+1][strlen(argv[i+1])-1]='\0';
					num = atol(argv[i+1]);
					if(num < 0 || num > 2147483647)
					{
						printf("\nPlease enter a valid n value\n");
						exit(1);
					}	
				}else{
					num = atol(argv[i+1]);
					if(num < 0 || num > 2147483647)
					{
						printf("\nPlease enter a valid n value\n");
						exit(1);
					}
				}
			}
			if(strcmp(argv[i],"-t")==0 || strcmp(argv[i],"[-t")==0){
			 	tf=0;
				if(argv[i+1][strlen(argv[i+1])-1]==']'){
					argv[i+1][strlen(argv[i+1])-1]='\0';
					readFile(argc,argv[i+1]);
					fileName = argv[i+1];
				}else{
					readFile(argc,argv[i+1]);
					fileName = argv[i+1];
				}
			readFile_flag=1;
			}
		}
		if(lf && muf && rf && pf && nf && bf && tf){
				printf("\nMalformed Command ! usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}
		}
		if(!readFile_flag){
			printf("\nEmulation Parameters:");
			printf("\n\tnumber to arrive = %ld",num);
			printf("\n\tlambda = %0.2lf",lambda);
			printf("\n\tmu = %0.2lf",mu_value);
			printf("\n\tr = %0.2lf",r_value);
			printf("\n\tB = %ld",bucketDepth);
			printf("\n\tP = %ld",reqTokensToPass);
		}else{
			printf("\nEmulation Parameters:");
			printf("\n\tnum to arrive = %ld",num);
			printf("\n\tr = %0.2lf",r_value);
			printf("\n\tB = %ld",bucketDepth);
			printf("\n\ttsfile = %s\n",fileName);
		}
		simu_startTime = getTimeStamp();
		printf("\n00000000.000ms: emulation begins");
		pthread_create(&packetArrivalThread,0,packetArrival,0);
		pthread_create(&tokenBucketThread,0,tokenBucket,0);
		pthread_create(&server2,0,server2Impl,0);
		pthread_create(&server1,0,server1Impl,0);
		pthread_create(&ctrlThread,0,signalHandler,0);
		pthread_join(packetArrivalThread,NULL);
		pthread_join(tokenBucketThread,NULL);
		pthread_join(server1,NULL);
		pthread_join(server2,NULL);
		pthread_cancel(ctrlThread);
		simu_endTime = getTimeStamp();
		printf("\n%012.3lfms: emulation ends",simu_endTime-simu_startTime);
		statistics();
		printf("\n");
		return 0;

}

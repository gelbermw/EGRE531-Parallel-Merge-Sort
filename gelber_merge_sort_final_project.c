/***********************************************
*
* Name: Matthew Gelber
* Assignment: EGRE 531 Final Project
* Date: May 7, 2021
*
***********************************************/

//compile with:
//gcc -g gelber_merge_sort_final_project.c -o final -lpthread -lm
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <pthread.h>

#define size 128000000
#define numThreads 2

static pthread_barrier_t barrier;
double overallTime;

double read_timer_ms() {
    struct timeb tm;
    ftime(&tm);
    return (double) tm.time * 1000.0 + (double) tm.millitm;
}

typedef struct my_array {
	int* data;
	int length;
} my_array;

typedef struct thread_args {
	my_array* all_data;
	int threadID;
	int threadsRemain;
} thread_args;

int peek(my_array a){
	return a.data[0];
}

void pop(my_array* a){
	a->data++;
	a->length--;
}

void printer(my_array a){
	printf("{");
	if(a.length != 0){
		printf("%d",a.data[0]);
		for(int i=1; i<a.length; i++)
			printf(",%d",a.data[i]);
	}
	printf("}\n");
}

my_array merge(my_array source1, my_array source2){
	my_array dest_array;
	my_array source1_temp = source1;
	my_array source2_temp = source2;
	int current_pos = 0;
	dest_array.length = source1.length + source2.length;
	dest_array.data = malloc(sizeof(int)*dest_array.length);

	while(source1.length > 0 && source2.length > 0){
		if(peek(source1) < peek(source2)){
			dest_array.data[current_pos] = peek(source1);
			pop(&source1);
			current_pos++;
		}
		else{
			dest_array.data[current_pos] = peek(source2);
			pop(&source2);
			current_pos++;
		}
	}
	if(source1.length == 0){
		memcpy(&dest_array.data[current_pos], source2.data, source2.length*sizeof(int));
	}
	if(source2.length == 0){
		memcpy(&dest_array.data[current_pos], source1.data, source1.length*sizeof(int));
	}
	
	free(source1_temp.data);
	free(source2_temp.data);
	
	return dest_array;
}

my_array divide(my_array input){
	if(input.length <= 1)
		return input;
		
	my_array a, b;
	a.length = input.length/2;
	b.length = input.length - a.length;
	
	a.data = malloc(sizeof(int)*a.length);
	b.data = malloc(sizeof(int)*b.length);
	
	memcpy(a.data, input.data, a.length*sizeof(int));
	memcpy(b.data, &input.data[a.length], b.length*sizeof(int));
	
	free(input.data);
	
	a = divide(a);
	b = divide(b);
		
	return merge(a, b);
}

void* divideSortThreads(void* args){
	int threadsBefore = 0;
	thread_args* localArgs = (thread_args*) args;
	if(localArgs->threadID == 0){
		overallTime = read_timer_ms();
	}
	localArgs->all_data[localArgs->threadID] = divide(localArgs->all_data[localArgs->threadID]);
	pthread_barrier_wait(&barrier);
	
	threadsBefore = localArgs->threadsRemain;
	localArgs->threadsRemain = localArgs->threadsRemain / 2;
	while(localArgs->threadsRemain >= 1){
		if((localArgs->threadID + localArgs->threadsRemain) < threadsBefore){
			localArgs->all_data[localArgs->threadID] = merge(localArgs->all_data[localArgs->threadID], localArgs->all_data[localArgs->threadID + localArgs->threadsRemain]);
		}
		
		pthread_barrier_wait(&barrier);
		threadsBefore = localArgs->threadsRemain;
		localArgs->threadsRemain = localArgs->threadsRemain / 2;
	}
	if(localArgs->threadID == 0){
		overallTime = (read_timer_ms() - overallTime) / 1000;
	}
}


int main(int argc, char *argv[]){

	printf("Number of elements to be sorted: %d\n",size);
	printf("Number of threads: %d\n\n",numThreads);
	
	static int array[size];
	int threadCounter = numThreads;
	clock_t startTime, endTime;
	
	srand(0);
	
	for(int i = 0; i < size; i++){
		array[i] = rand() % 2000;
	}
		
	my_array* array_of_arrays = malloc(sizeof(my_array)*threadCounter);	
	
	struct thread_args *thread_data = malloc(sizeof(struct thread_args)*threadCounter);
	
	pthread_t sortThreads[threadCounter];

	int start = 0, end = 0;
	int i = 0;
	for (i=0; i<threadCounter; i++){
		end = start + (int)(ceil((double)size/(double)threadCounter) - 1);
		end = end>=size?(size-1):end;
		array_of_arrays[i].length = end-start+1;
		array_of_arrays[i].data = malloc(sizeof(int)*array_of_arrays[i].length);
		memcpy(array_of_arrays[i].data, &array[start], array_of_arrays[i].length * sizeof(int));
		start = end + 1;
	}
		
	pthread_barrier_init(&barrier, NULL, threadCounter);
		
	for(i=0; i<threadCounter; i++){
		thread_data[i].all_data = array_of_arrays;
		thread_data[i].threadID = i;	//create for loop for creating arrays
		thread_data[i].threadsRemain = threadCounter;
		pthread_create(&sortThreads[i], NULL, &divideSortThreads, (void *)&thread_data[i]);
	}
	
	for(i=0; i<threadCounter; i++){
		pthread_join(sortThreads[i], NULL);		//sync threads
	}
	
	
	printf("Total runtime = %f\n",overallTime);
		
	free(array_of_arrays);
	free(thread_data);
}

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>
#include <stdlib.h> 
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define N 5
#define tst 1
#define latest_arrival_time 15
#define maximum_computation_time 8
#define maximum_period_multipler 17

struct task
{
	int task_number;
	int Ta;
	int Tc;
	int Tp;
	float cpu_utilization;
	float rate;
	int priority;
	bool isCreated;
	bool isDeleted;
};

struct task *Tasks;
int n;
bool idleRunning = true;
portTickType xLastWakeTime;
xTaskHandle* handleArray;

int Random(int lower_bound, int upper_bound);
void task_function(void* i);

void calcPriority(){
	int prev_period = 0;
	int priority = 1;
	for (int i = 0; i < n; i++) {
		if(Tasks[i].isDeleted)
			continue;
		if (Tasks[i].Tp == prev_period) {
			Tasks[i].priority = priority-1;
		}
		else {
			Tasks[i].priority = priority++;
			prev_period = Tasks[i].Tp;
		}
		if(Tasks[i].isCreated)
			vTaskPrioritySet(handleArray[Tasks[i].task_number] ,Tasks[i].priority);
	}	
}

void randTaskDelete(void){
	int rand = Random(0,400000);
	if(rand == 8){
		int i = Random(0,n-1);
		if(Tasks[i].isDeleted || !Tasks[i].isCreated)  	//check if task is already deleted or is not created yet
			return;
		vTaskSuspendAll();		// suspend the scheduler
		Tasks[i].isDeleted = true;
		vTaskDelete(handleArray[Tasks[i].task_number]);
		calcPriority();
		printf("Task %d is deleted \n", Tasks[i].task_number);
		xTaskResumeAll();			//restart scheduler
	}	
}

void taskArrival(void){
	char task_name[12];
	for(int i = 0; i < n; i++){ 
		xLastWakeTime = xTaskGetTickCount();
		if(((int)(xLastWakeTime/1000) >= Tasks[i].Ta) && (Tasks[i].isCreated == false)){ //check if task arrived based on its arrival time
			Tasks[i].isCreated = true;
			snprintf(task_name, 12, "Task %d", Tasks[i].task_number); // concatenates task with iteration number +1
			printf("Task %d is created at ", Tasks[i].task_number);
			printf("%d seconds\n",Tasks[i].Ta);
			xTaskCreate(task_function, task_name, configMINIMAL_STACK_SIZE, (void *)i, Tasks[i].priority, &handleArray[Tasks[i].task_number]);
		}
	}
}

void task_function(void* i) {
		unsigned long compStart;
		int count;
		for ( ;; ) {
			xLastWakeTime = xTaskGetTickCount();
			compStart = xLastWakeTime;
			count = xLastWakeTime;
			printf("Task %d is running for %d seconds\n", Tasks[(int)i].task_number, Tasks[(int)i].Tc);
			while(((int)((xLastWakeTime - compStart)/1000)) < Tasks[(int)i].Tc){ //wait for computaion time to pass
				if((int)(xLastWakeTime - count)/1000 == 1){
					printf("Task %d is running for %d seconds\n", Tasks[(int)i].task_number, Tasks[(int)i].Tc - (int)((xLastWakeTime - compStart)/1000));					
					count += 1000;
				}
				randTaskDelete();
				taskArrival();
			}
			idleRunning = true;
			printf("Task %d is delaying for %d seconds\n", Tasks[(int)i].task_number, Tasks[(int)i].Tp);
			vTaskDelayUntil(&xLastWakeTime, (Tasks[(int)i].Tp*1000)/portTICK_RATE_MS);
	}
}

void vApplicationIdleHook(void){
	for( ;; ){
		if(idleRunning){
			printf("Idle task is running\n");
			idleRunning = false;
		}
		randTaskDelete();
		taskArrival();
	}
}

void swap(struct task* a, struct task* b)
{
	struct task t = *a;
	*a = *b;
	*b = t;
}

int partition(struct task Tasks[N], int low, int high)
{
	float pivot = Tasks[high].rate;    // pivot 
	int i = (low - 1);  // Index of smaller element 

	for (int j = low; j <= high - 1; j++)
	{
		// If current element is smaller than the pivot 
		if (Tasks[j].rate < pivot)
		{
			i++;    // increment index of smaller element 
			swap(&Tasks[i], &Tasks[j]);
		}
	}
	swap(&Tasks[i + 1], &Tasks[high]);
	return (i + 1);
}

void quickSort(struct task Tasks[N], int low, int high)
{
	if (low < high)
	{
		/* pi is partitioning index, arr[p] is now
		at right place */
		int pi = partition(Tasks, low, high);

		// Separately sort elements before 
		// partition and after partition 
		quickSort(Tasks, low, pi - 1);
		quickSort(Tasks, pi + 1, high);
	}
}

struct task init_Task(int number, int arrival, int computation, int period ,float utilization, float rt, int prt)
{
	struct task t =
	{ number, arrival, computation, period, utilization, rt, prt, false, false};
	return t;
}

int Random(int lower_bound, int upper_bound)
{
	int n = (rand() % (upper_bound - lower_bound + 1)) + lower_bound;
	return n;
}

bool schedulablity_test() {
	float Ucpu = 0;
	for (int i = 0; i < n; i++) {
		Ucpu += Tasks[i].cpu_utilization;
	}
	if (Ucpu < 0.7) {
		printf("schedulablity check is ok \n");
		return true;
	}
	else {
		printf("schedulablity check is not ok \n");
		return false;
	}
}


int main(void)
{
	bool isSchedulable = false;
	srand(1);
	n = Random(2, N);
	handleArray = malloc(n * sizeof(int));
	Tasks = malloc(n * sizeof(int));
	while (isSchedulable == false) {
		for (int i = 0; i < n; i++)
		{
			int Ta = Random(0, latest_arrival_time);
			int Tc = Random(1, maximum_computation_time);
			int Tp = Random(3, maximum_period_multipler) * Tc;
			Tasks[i] = init_Task(i, Ta, Tc, Tp, ((float)Tc / Tp), ((float)1 / Tp), 0);
		}
		isSchedulable = schedulablity_test();
	}
	quickSort(Tasks, 0, n - 1);
	calcPriority();
	vTaskStartScheduler();
	return 0;
}

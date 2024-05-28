
/* FreeRTOS.org includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Demo includes. */
#include "supporting_functions.h"


#include "string.h"

#include "FreeRTOS.h"
#include "task.h"
#include "stdlib.h"
#include "semphr.h"

#include "timers.h"

typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

#define DEPARTMENTS_NUM 4
#define MAX_NUM_REQUESTED_VEHICLES 10
#define MAX_SR_HANDLES 10 // max number of handles for sr threads in each department

#define INIT_POLICE_NUM 10
#define INIT_AMBULANCE_NUM 10
#define INIT_FIRE_NUM 10
#define INIT_CORONA_NUM 10
#define CORONA_CONCURRENT_NUM 10
#define AMBULANCE_CONCURRENT_NUM 10
#define POLICE_CONCURRENT_NUM 10
#define FIRE_CONCURRENT_NUM 10
#define INIT_DISPATCHER_SIZE 50
#define DISPATCH_INIT_BUFF_SIZE_BYTES 10
#define AMBULANCE_INIT_BUFF_SIZE_BYTES 10
#define FIRE_INIT_BUFF_SIZE_BYTES 10
#define CORONA_INIT_BUFF_SIZE_BYTES 10
#define POLICE_INIT_BUFF_SIZE_BYTES 10
#define LOG_TIMER_INTERVAL 1000
#define DISPATCH_QUEUE_TIMEOUT portMAX_DELAY
#define POLICE_QUEUE_TIMEOUT portMAX_DELAY
#define FIRE_QUEUE_TIMEOUT portMAX_DELAY
#define AMBULANCE_QUEUE_TIMEOUT portMAX_DELAY
#define CORONA_QUEUE_TIMEOUT portMAX_DELAY
#define LOG_TIMER_INTERVAL 1000
#define XTICKS_MAX 500
#define LOGGING_THREAD_PRIORITY (1)
#define LOG_THREAD_BUFFER_SIZE (128)

/* Define for GenerateThread */
#define GENERATE_THREAD_PRIORITY (1)
#define GENERATE_THREAD_BUFFER_SIZE (128)

/* Define for DispatchThread */
#define DISPATCH_THREAD_PRIORITY (1)
#define DISPATCH_THREAD_BUFFER_SIZE (128)

/* Define for PoliceThread */
#define POLICE_THREAD_PRIORITY (1)
#define POLICE_THREAD_BUFFER_SIZE (128)

/* Define for FireThread */
#define FIRE_THREAD_PRIORITY (1)
#define FIRE_THREAD_BUFFER_SIZE (128)

/* Define for AmbulanceThread */
#define AMBULANCE_THREAD_PRIORITY (1)
#define AMBULANCE_THREAD_BUFFER_SIZE (128)

/* Define for CoronaThread */
#define CORONA_THREAD_PRIORITY (1)
#define CORONA_THREAD_BUFFER_SIZE (128)

#define AMBULANCE_SR_THREAD_PRIORITY (2)
#define AMBULANCE_SR_THREAD_BUFFER_SIZE (128)

#define POLICE_SR_THREAD_PRIORITY (2)
#define POLICE_SR_THREAD_BUFFER_SIZE (128)

#define CORONA_SR_THREAD_PRIORITY (2)
#define CORONA_SR_THREAD_BUFFER_SIZE (128)

#define FIRE_SR_THREAD_PRIORITY (2)
#define FIRE_SR_THREAD_BUFFER_SIZE (128)

#define SEMAPHORE_WAIT_TIME portMAX_DELAY

//TaskFunction_t loggingStartThread(void* argument);

//TaskFunction_t timerCallback(void* argument);
TaskFunction_t ServiceRoutine(void* req);

TaskFunction_t departmentTask(void* argument);

TaskFunction_t dispatchTask(void* argument);
TaskFunction_t generateTask(void* argument);



uint32_t totalVehicles = MAX_NUM_REQUESTED_VEHICLES;


BaseType_t department_thread_priority[DEPARTMENTS_NUM] = { POLICE_SR_THREAD_PRIORITY, FIRE_SR_THREAD_PRIORITY, AMBULANCE_SR_THREAD_PRIORITY, CORONA_SR_THREAD_PRIORITY };
typedef enum {
    POLICE,
    FIRE,
    AMBULANCE,
    CORONA
}department_id;

typedef struct {
    uint8_t init_num;
    uint8_t concurrent_num;
    uint8_t available_num;

}department_data;

typedef struct {
    department_id dep_id;
    uint8_t requested_vehicles;
    uint32_t time_to_complete;
}request;

typedef struct {
    uint8_t available_num;
}dispatcher_data;
void releaseResources(int i, department_id depar_id);

department_data police_dep = { INIT_POLICE_NUM, POLICE_CONCURRENT_NUM, POLICE_CONCURRENT_NUM };
department_data fire_dep = { INIT_FIRE_NUM, FIRE_CONCURRENT_NUM, FIRE_CONCURRENT_NUM };
department_data ambulance_dep = { INIT_AMBULANCE_NUM, AMBULANCE_CONCURRENT_NUM, AMBULANCE_CONCURRENT_NUM };
department_data corona_dep = { INIT_CORONA_NUM, CORONA_CONCURRENT_NUM, CORONA_CONCURRENT_NUM };
department_data department_list[DEPARTMENTS_NUM] = { 0 };
dispatcher_data dispatch_data = { INIT_DISPATCHER_SIZE };

SemaphoreHandle_t policeSemaphore;
SemaphoreHandle_t globalSemaphore;

SemaphoreHandle_t ambulanceSemaphore;
SemaphoreHandle_t fireSemaphore;
SemaphoreHandle_t coronaSemaphore;

/* Definitions for loggingThread */
TaskHandle_t loggingThreadHandle;


/* Definitions for GenerateThread */
TaskHandle_t generateThreadHandle;

/* Definitions for DispatchThread */


TaskHandle_t dispatchThreadHandle;


/* Definitions for PoliceThread */
TaskHandle_t policeThreadHandle;

/* Definitions for FireThread */
TaskHandle_t fireThreadHandle;


/* Definitions for AmbulanceThread */
TaskHandle_t ambulanceThreadHandle;


/* Definitions for CoronaThread */
TaskHandle_t coronaThreadHandle;

/* Definitions for DispatchQueue */
QueueHandle_t DispatchQueueHandle;

/* Definitions for AmbulanceQueue */
QueueHandle_t AmbulanceQueueHandle;

/* Definitions for PoliceQueue */
QueueHandle_t PoliceQueueHandle;

/* Definitions for FireQueue */
QueueHandle_t FireQueueHandle;

/* Definitions for CoronaQueue */
QueueHandle_t CoronaQueueHandle;


QueueHandle_t department_queue_handles_lists[DEPARTMENTS_NUM];
TickType_t QueueTimeoutList[DEPARTMENTS_NUM] = { POLICE_QUEUE_TIMEOUT,FIRE_QUEUE_TIMEOUT,AMBULANCE_QUEUE_TIMEOUT,CORONA_QUEUE_TIMEOUT };

//TaskFunction_t loggingStartThread(void* argument);
SemaphoreHandle_t semaphoreList[DEPARTMENTS_NUM];

int main(void)
{

    department_list[POLICE] = police_dep;
    department_list[FIRE] = fire_dep;
    department_list[AMBULANCE] = ambulance_dep;
    department_list[CORONA] = corona_dep;

    policeSemaphore = xSemaphoreCreateCounting(
        MIN(POLICE_CONCURRENT_NUM, INIT_POLICE_NUM),
        MIN(POLICE_CONCURRENT_NUM, INIT_POLICE_NUM)
    );
    printf("police semaphore count: %d\n", uxSemaphoreGetCount(policeSemaphore));
    fireSemaphore = xSemaphoreCreateCounting(
        MIN(FIRE_CONCURRENT_NUM, INIT_FIRE_NUM),
        MIN(FIRE_CONCURRENT_NUM, INIT_FIRE_NUM)
    );
    printf("fire semaphore count: %d\n", uxSemaphoreGetCount(fireSemaphore));

    ambulanceSemaphore = xSemaphoreCreateCounting(
        MIN(AMBULANCE_CONCURRENT_NUM, INIT_AMBULANCE_NUM),
        MIN(AMBULANCE_CONCURRENT_NUM, INIT_AMBULANCE_NUM)
    );
    printf("ambulance semaphore count: %d\n", uxSemaphoreGetCount(ambulanceSemaphore));

    coronaSemaphore = xSemaphoreCreateCounting(
        MIN(CORONA_CONCURRENT_NUM, INIT_CORONA_NUM),
        MIN(CORONA_CONCURRENT_NUM, INIT_CORONA_NUM)
    );
    printf("corona semaphore count: %d\n", uxSemaphoreGetCount(coronaSemaphore));

    globalSemaphore = xSemaphoreCreateCounting(MAX_NUM_REQUESTED_VEHICLES,
        MAX_NUM_REQUESTED_VEHICLES);
    printf("global semaphore count: %d\n", uxSemaphoreGetCount(globalSemaphore));


    semaphoreList[0] = policeSemaphore;
    semaphoreList[1] = fireSemaphore;

    semaphoreList[2] = ambulanceSemaphore;

    semaphoreList[3] = coronaSemaphore;



    /* Create the queue(s) */
    /* creation of DispatchQueue */
    DispatchQueueHandle = xQueueCreate(DISPATCH_INIT_BUFF_SIZE_BYTES, sizeof(request));

    /* creation of AmbulanceQueue */
    AmbulanceQueueHandle = xQueueCreate(AMBULANCE_INIT_BUFF_SIZE_BYTES, sizeof(request));

    /* creation of PoliceQueue */
    PoliceQueueHandle = xQueueCreate(POLICE_INIT_BUFF_SIZE_BYTES, sizeof(request));

    /* creation of FireQueue */
    FireQueueHandle = xQueueCreate(FIRE_INIT_BUFF_SIZE_BYTES, sizeof(request));

    /* creation of CoronaQueue */
    CoronaQueueHandle = xQueueCreate(CORONA_INIT_BUFF_SIZE_BYTES, sizeof(request));


    /* creation of loggingThread */
    //BaseType_t log_thread = xTaskCreate(loggingStartThread, (const char*)"loggingStartThread", (const void*)LOG_THREAD_BUFFER_SIZE, NULL, LOGGING_THREAD_PRIORITY, (const TaskHandle_t*)&loggingThreadHandle);
    department_id corona_depar = CORONA;
    department_id fire_depar = FIRE;
    department_id police_depar = POLICE;
    department_id ambulance_depar = AMBULANCE;
    /* USER CODE BEGIN RTOS_THREADS */
    BaseType_t generate_thread = xTaskCreate(generateTask, (const char*)"generateThread", (const void*)GENERATE_THREAD_BUFFER_SIZE, NULL, GENERATE_THREAD_PRIORITY, (const TaskHandle_t*)&generateThreadHandle);
    BaseType_t dispatch_thread = xTaskCreate(dispatchTask, (const char*)"dispatchThread", (const void*)DISPATCH_THREAD_BUFFER_SIZE, NULL, DISPATCH_THREAD_PRIORITY, (const TaskHandle_t*)&dispatchThreadHandle);

    BaseType_t police_thread = xTaskCreate(departmentTask, (const char*)"policeThread", (const void*)POLICE_THREAD_BUFFER_SIZE, (void*)&police_depar, POLICE_THREAD_PRIORITY, (const TaskHandle_t*)&policeThreadHandle);

    BaseType_t fire_thread = xTaskCreate(departmentTask, (const char*)"fireThread", (const void*)FIRE_THREAD_BUFFER_SIZE, (void*)&fire_depar, FIRE_THREAD_PRIORITY, (const TaskHandle_t*)&fireThreadHandle);
    BaseType_t ambulnace_thread = xTaskCreate(departmentTask, (const char*)"ambulanceThread", (const void*)AMBULANCE_THREAD_BUFFER_SIZE, (void*)&ambulance_depar, AMBULANCE_THREAD_PRIORITY, (const TaskHandle_t*)&ambulanceThreadHandle);
    BaseType_t corona_thread = xTaskCreate(departmentTask, (const char*)"coronaThread", (const void*)CORONA_THREAD_BUFFER_SIZE, (void*)&corona_depar, CORONA_THREAD_PRIORITY, (const TaskHandle_t*)&coronaThreadHandle);


    vTaskStartScheduler();
    //never reach here
    while (1)
    {

    }

}


// @brief: takes request from dispatcher queue and send to relevant department queue
TaskFunction_t dispatchTask(void* argument) {



    BaseType_t retval_Sent_From_Dispatch_Queue = 0;
    BaseType_t retval_Police_Send = 0;
    BaseType_t retval_Fire_Send = 0;
    BaseType_t retval_Corona_Send = 0;
    BaseType_t retval_Ambulance_Send = 0;

    BaseType_t retval_Send_to_dep_list[DEPARTMENTS_NUM] = { retval_Police_Send,retval_Fire_Send, retval_Ambulance_Send,retval_Corona_Send };
    department_queue_handles_lists[POLICE] = PoliceQueueHandle;
    department_queue_handles_lists[FIRE] = FireQueueHandle;
    department_queue_handles_lists[AMBULANCE] = AmbulanceQueueHandle;
    department_queue_handles_lists[CORONA] = CoronaQueueHandle;



    request req = { 0 };
    while (1) {
        if ((retval_Sent_From_Dispatch_Queue = xQueueReceive(DispatchQueueHandle, &req, DISPATCH_QUEUE_TIMEOUT)) == pdPASS) {
            department_id id = req.dep_id;

            //printf("dispatcher got request: dep_id: %d, requested vehicles: %u, time to comlete: %u\n", req.dep_id, req.requested_vehicles, req.time_to_complete);


            if ((retval_Send_to_dep_list[id] = xQueueSendToBack(department_queue_handles_lists[id], &req, QueueTimeoutList[id])) == pdPASS) {
                //printf("request sent to department!\n");
                switch (id) {
                case FIRE:
                    //printf("Fire request was sent to department!\r\n");
                    break;
                case AMBULANCE:
                    //printf("Ambulance request was sent to department!\r\n");
                    break;
                case CORONA:
                    //printf("Corona request was sent to department!\r\n");
                    break;
                case POLICE:
                    //printf("Police request was sent to department!\r\n");
                    break;
                default:
                    break;


                }
            }
            else {
                //printf(" dispatcher sending request to department %d failed. time passed or queue is full!\n", (int)id);

            }
        }
        else {
            //printf(" dispatcher couldnt get request. receiving request from main queue was failed. time passed or queue is empty!\n");

        }
    }
}


// @brief: this function takes request from department queue and send to execute thread
TaskFunction_t departmentTask(void* dep_id) {//argument is speartment_id

    department_id depar_id = *((department_id*)dep_id);
    //printf("entered %d department\n", (int)depar_id);
    BaseType_t retval_SR_thread;
    request req;
    int thread_buffer_list[DEPARTMENTS_NUM] = { POLICE_SR_THREAD_BUFFER_SIZE, FIRE_SR_THREAD_BUFFER_SIZE, AMBULANCE_SR_THREAD_BUFFER_SIZE,CORONA_SR_THREAD_BUFFER_SIZE };
    TaskHandle_t SR_handle_array[MAX_SR_HANDLES] = { 0 };
    uint8_t sr_handle_index = 0;

    while (1)
    {
        sr_handle_index %= MAX_SR_HANDLES;
        if (xQueueReceive(department_queue_handles_lists[depar_id], (void const*)&req, QueueTimeoutList[depar_id]) == pdPASS)
        {
            printf(" department %d received request\n", (int)depar_id);
            switch (depar_id) {
            case FIRE:
                //printf("Got request from fire department sending to SR!\r\n");
                break;
            case AMBULANCE:
                //printf("Got request from ambulance department sending to SR!\r\n");
                break;
            case POLICE:
                //printf("Got request from police department sending to SR!\r\n");
                break;
            case CORONA:
                //printf("Got request from corona department sending to SR!\r\n");
                break;
            }
            if ((retval_SR_thread = xTaskCreate(
                ServiceRoutine,
                (const char*)"ServiceRoutineThread",
                thread_buffer_list[depar_id],
                (void const*)&req, //request to be passed
                department_thread_priority[depar_id],
                NULL/*(const TaskHandle_t*)&SR_handle_array[sr_handle_index]*/)) == pdPASS) //should i add handles to each of task created?
            {
                //printf("New SR was created!\r\n");
                sr_handle_index++;
            }
            else { 
                //printf("NEW SR failed to be created!\n"); 
            }
        }
        else {
            //printf("receive request from %d department queue failed. queue empty or time passed!\n", (int)depar_id);

        }
    }
}

TaskFunction_t ServiceRoutine(void* req) {
    request* reques = (request*)req;//
    department_id depar_id = reques->dep_id;
    int glob_semaphore_count_before_1 = uxSemaphoreGetCount(globalSemaphore);

    SemaphoreHandle_t department_semaphore = semaphoreList[depar_id];
    printf("department request %d entered service routine\n", depar_id);
    int i = 0;
    printf("requested cars: %d, sempahoreglobal: %d, concurrentsemaphore: %d", reques->requested_vehicles, uxSemaphoreGetCount(globalSemaphore), uxSemaphoreGetCount(department_semaphore));
    while (i < reques->requested_vehicles) {
        printf("global sempahore count: %ld, concurrent semaphore count: %ld", uxSemaphoreGetCount(globalSemaphore), uxSemaphoreGetCount(department_semaphore));
        int glob_semaphore_count_before_2 = uxSemaphoreGetCount(globalSemaphore);
        if (xSemaphoreTake(globalSemaphore, 0) == pdPASS && xSemaphoreTake(semaphoreList[depar_id], 0) == pdPASS) {
            printf("global semaphore and depar semaphore were taken! priority is %d\n", uxTaskPriorityGet(NULL));
           // vTaskPrioritySet(NULL, uxTaskPriorityGet(NULL) + 1);// increases prioirity by one
            i++;
            int glob_semaphore_count_before_3 = uxSemaphoreGetCount(globalSemaphore);
            totalVehicles--;
            department_list[depar_id].available_num--;
            department_list[depar_id].concurrent_num--;
            /* in case all resources for execution acquired, exectues, releases all semaphores and end task*/
            if (i == reques->requested_vehicles - 1) {
                printf("##################################sending vehicles!\n");
                //vTaskDelay(pdMS_TO_TICKS(reques->time_to_complete));
                releaseResources(i, depar_id);
                printf("released resources!\n");
                int glob_semaphore_count_before_4 = uxSemaphoreGetCount(globalSemaphore);
                vTaskDelete(NULL);
            }
        }
        else {
            /* in deadlock releases all semaphores acquired and then starts again*/
            releaseResources(i, depar_id);
           // vTaskPrioritySet(NULL, (UBaseType_t)department_thread_priority[depar_id]);
            int glob_semaphore_count_before_5 = uxSemaphoreGetCount(globalSemaphore);
            printf("collision detected, released all semaphores!\n");
            i = 0;


        }
    }
}



void releaseResources(int i, department_id depar_id) {
    for (int j = 0; j < i;j++) {
        int glob_semaphore_count_before = uxSemaphoreGetCount(globalSemaphore);
        xSemaphoreGive(globalSemaphore);
        int glob_semahpore_count_after = uxSemaphoreGetCount(globalSemaphore);
        xSemaphoreGive(semaphoreList[depar_id]);
        printf("department semaphore and global were given!\n");
        totalVehicles++;
        department_list[depar_id].concurrent_num++;
        department_list[depar_id].available_num++;
        printf("released resources of %d department: totalVehicles: %d, concurrent num: %d, availble num: %d\n", depar_id, totalVehicles, department_list[depar_id].concurrent_num, department_list[depar_id].available_num);
        //to add delay time to break symmetry?
    }
}








TaskFunction_t timerCallback(void* argument) {
    printf("Dispatcher initial size: %d\r\n", INIT_DISPATCHER_SIZE);
    printf("Dispatcher free size: %d\r\n", dispatch_data.available_num);
    printf("Dispatcher occupied size: %d\r\n", INIT_DISPATCHER_SIZE - dispatch_data.available_num);



}

TaskFunction_t loggingStartThread(void* argument)
{

    TimerHandle_t log_timer = xTimerCreate("LogTimer", pdMS_TO_TICKS(LOG_TIMER_INTERVAL), pdTRUE, 0, timerCallback);
    if (log_timer != NULL) {
        // Start the timer
        xTimerStart(log_timer, 0);
    }

}



TaskFunction_t generateTask(void* argument)
{
    BaseType_t retval_Send_To_Dispatch_Queue = 0;
    while (1) {
        department_id dep = rand() % DEPARTMENTS_NUM;
        uint32_t active_time = 1;//;rand() % XTICKS_MAX;
        uint8_t vehicle_num_to_dispatch = (rand() % MAX_NUM_REQUESTED_VEHICLES) + 1; // request num of vehicles can be only greater than zero
        request req = { dep,vehicle_num_to_dispatch,active_time };
        if ((retval_Send_To_Dispatch_Queue = xQueueSendToBack(DispatchQueueHandle, &req, DISPATCH_QUEUE_TIMEOUT)) == pdPASS) {
            printf("generated request. request was put inside main queue! request: dep: %d, active time: %u, vehicle num: %u\r\n", dep, active_time, vehicle_num_to_dispatch);

        }
        else {
            printf("request couldnt enter main queue due to it is full or time passed!\r\n");
        }


    }
}

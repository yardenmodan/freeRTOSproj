/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

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
#define MAX_NUM_REQUESTED_VEHICLES 5

#define INIT_POLICE_NUM 10
#define INIT_AMBULANCE_NUM 10
#define INIT_FIRE_NUM 10
#define INIT_CORONA_NUM 10
#define CORONA_CONCURRENT_NUM 4
#define AMBULANCE_CONCURRENT_NUM 4
#define POLICE_CONCURRENT_NUM 3
#define FIRE_CONCURRENT_NUM 2
#define INIT_DISPATCHER_SIZE 50
#define DISPATCH_INIT_BUFF_SIZE_BYTES 1024
#define AMBULANCE_INIT_BUFF_SIZE_BYTES 1024
#define FIRE_INIT_BUFF_SIZE_BYTES 1024
#define CORONA_INIT_BUFF_SIZE_BYTES 1024
#define POLICE_INIT_BUFF_SIZE_BYTES 1024
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

#define AMBULANCE_SR_THREAD_PRIORITY (1)
#define AMBULANCE_SR_THREAD_BUFFER_SIZE (128)

#define POLICE_SR_THREAD_PRIOIRTY (1)
#define POLICE_SR_THREAD_BUFFER_SIZE (128)

#define CORONA_SR_THREAD_PRIOIRTY (1)
#define CORONA_SR_THREAD_BUFFER_SIZE (128)

#define FIRE_SR_THREAD_PRIOIRTY (1)
#define FIRE_SR_THREAD_BUFFER_SIZE (128)

#define SEMAPHORE_WAIT_TIME portMAX_DELAY

TaskFunction_t loggingStartThread(void* argument);

TaskFunction_t timerCallback(void* argument);
TaskFunction_t ServiceRoutine(void* req);

TaskFunction_t departmentTask(void* argument);

TaskFunction_t dispatchTask(void* argument);
TaskFunction_t generateTask(void* argument);
void releaseResources(int i, department_id depar_id);



uint32_t totalVehicles = MAX_NUM_REQUESTED_VEHICLES;
uint32_t coronaConNum = CORONA_CONCURRENT_NUM;
uint32_t policeConNum = POLICE_CONCURRENT_NUM;
uint32_t fireConNum = FIRE_CONCURRENT_NUM;
uint32_t ambulanceConNum = AMBULANCE_CONCURRENT_NUM;

BaseType_t department_thread_priority[DEPARTMENTS_NUM] = { POLICE_THREAD_PRIORITY, FIRE_THREAD_PRIORITY, AMBULANCE_THREAD_PRIORITY, CORONA_THREAD_PRIORITY };
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
    TickType_t time_to_complete;
}request;

typedef struct {
    uint8_t available_num;
}dispatcher_data;
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

TaskFunction_t loggingStartThread(void* argument);
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
    fireSemaphore = xSemaphoreCreateCounting(
        MIN(FIRE_CONCURRENT_NUM, INIT_FIRE_NUM),
        MIN(FIRE_CONCURRENT_NUM, INIT_FIRE_NUM)
    );
    ambulanceSemaphore = xSemaphoreCreateCounting(
        MIN(AMBULANCE_CONCURRENT_NUM, INIT_AMBULANCE_NUM),
        MIN(AMBULANCE_CONCURRENT_NUM, INIT_AMBULANCE_NUM)
    );

    coronaSemaphore = xSemaphoreCreateCounting(
        MIN(CORONA_CONCURRENT_NUM, INIT_CORONA_NUM),
        MIN(CORONA_CONCURRENT_NUM, INIT_CORONA_NUM)
    );

    globalSemaphore = xSemaphoreCreateCounting(MAX_NUM_REQUESTED_VEHICLES,
        MAX_NUM_REQUESTED_VEHICLES);

    SemaphoreHandle_t semaphoreList[DEPARTMENTS_NUM] = { policeSemaphore,fireSemaphore,ambulanceSemaphore,coronaSemaphore };

    /* Create the queue(s) */
    /* creation of DispatchQueue */
    DispatchQueueHandle = xQueueCreate(DISPATCH_INIT_BUFF_SIZE_BYTES, sizeof(uint32_t));

    /* creation of AmbulanceQueue */
    AmbulanceQueueHandle = xQueueCreate(AMBULANCE_INIT_BUFF_SIZE_BYTES, sizeof(uint32_t));

    /* creation of PoliceQueue */
    PoliceQueueHandle = xQueueCreate(POLICE_INIT_BUFF_SIZE_BYTES, sizeof(uint32_t));

    /* creation of FireQueue */
    FireQueueHandle = xQueueCreate(FIRE_INIT_BUFF_SIZE_BYTES, sizeof(uint32_t));

    /* creation of CoronaQueue */
    CoronaQueueHandle = xQueueCreate(CORONA_INIT_BUFF_SIZE_BYTES, sizeof(uint32_t));


    /* creation of loggingThread */
    BaseType_t log_thread = xTaskCreate(loggingStartThread, (const char*)"loggingStartThread", (const void*)LOG_THREAD_BUFFER_SIZE, NULL, LOGGING_THREAD_PRIORITY, (const TaskHandle_t*)&loggingThreadHandle);

    /* USER CODE BEGIN RTOS_THREADS */
    BaseType_t generate_thread = xTaskCreate(generateTask, (const char*)"generateThread", (const void*)GENERATE_THREAD_BUFFER_SIZE, NULL, GENERATE_THREAD_PRIORITY, (const TaskHandle_t*)&generateThreadHandle);
    BaseType_t dispatch_thread = xTaskCreate(dispatchTask, (const char*)"dispatchThread", (const void*)DISPATCH_THREAD_BUFFER_SIZE, NULL, DISPATCH_THREAD_PRIORITY, (const TaskHandle_t*)&dispatchThreadHandle);

    BaseType_t police_thread = xTaskCreate(departmentTask, (const char*)"policeThread", (const void*)POLICE_THREAD_BUFFER_SIZE, POLICE, POLICE_THREAD_PRIORITY, (const TaskHandle_t*)&policeThreadHandle);

    BaseType_t fire_thread = xTaskCreate(departmentTask, (const char*)"fireThread", (const void*)FIRE_THREAD_BUFFER_SIZE, FIRE, FIRE_THREAD_PRIORITY, (const TaskHandle_t*)&fireThreadHandle);
    BaseType_t ambulnace_thread = xTaskCreate(departmentTask, (const char*)"ambulanceThread", (const void*)AMBULANCE_THREAD_BUFFER_SIZE, AMBULANCE, AMBULANCE_THREAD_PRIORITY, (const TaskHandle_t*)&ambulanceThreadHandle);
    BaseType_t corona_thread = xTaskCreate(departmentTask, (const char*)"coronaThread", (const void*)CORONA_THREAD_BUFFER_SIZE, CORONA, CORONA_THREAD_PRIORITY, (const TaskHandle_t*)&coronaThreadHandle);


    vTaskStartScheduler();

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



    request req;
    while (1) {
        if ((retval_Sent_From_Dispatch_Queue = xQueueReceive(DispatchQueueHandle, &req, DISPATCH_QUEUE_TIMEOUT)) == pdPASS) {

            department_id id = req.dep_id;

            if ((retval_Send_to_dep_list[id] = xQueueSend(department_queue_handles_lists[id], &req, QueueTimeoutList[id])) == pdPASS) {
                switch (id) {
                case FIRE:
                    printf("Fire request was sent to department!\r\n");
                    break;
                case AMBULANCE:
                    printf("Ambulance request was sent to department!\r\n");
                    break;
                case CORONA:
                    printf("Corona request was sent to department!\r\n");
                    break;
                case POLICE:
                    printf("Police request was sent to department!\r\n");
                    break;
                default:
                    break;


                }
            }
        }
    }
}


// @brief: this function takes request from department queue and send to execute thread
TaskFunction_t departmentTask(void* dep_id) //argument is speartment_id
{
    department_id depar_id = *((department_id*)dep_id);
    TaskHandle_t policeSRHandle;
    TaskHandle_t ambulanceSRHandle;

    TaskHandle_t fireSRHandle;

    TaskHandle_t coronaSRHandle;

    BaseType_t retval_SR_thread;
    request police_req;
    request ambulance_req;
    request fire_req;
    request corona_req;
    request departments_req_list[DEPARTMENTS_NUM] = { police_req,fire_req,ambulance_req,corona_req };
    int thread_buffer_list[DEPARTMENTS_NUM] = { POLICE_SR_THREAD_BUFFER_SIZE, FIRE_SR_THREAD_BUFFER_SIZE, AMBULANCE_SR_THREAD_BUFFER_SIZE,CORONA_SR_THREAD_BUFFER_SIZE };
    TaskHandle_t department_SR_handles[DEPARTMENTS_NUM] = { policeSRHandle, fireSRHandle, ambulanceSRHandle, coronaSRHandle };



    while (1)
    {
        if (xQueueReceive(department_queue_handles_lists[depar_id], (void const*)&departments_req_list[depar_id], QueueTimeoutList[depar_id]) == pdPASS)
        {
            switch (depar_id) {
            case FIRE:
                printf("Got request from fire department!\r\n");
                break;
            case AMBULANCE:
                printf("Got request from ambulance department!\r\n");
                break;
            case POLICE:
                printf("Got request from police department!\r\n");
                break;
            case CORONA:
                printf("Got request from corona department!\r\n");
                break;
            }
            if ((retval_SR_thread = xTaskCreate(
                ServiceRoutine,
                (const char*)"ServiceRoutineThread",
                thread_buffer_list[depar_id],
                (void const*)&departments_req_list[depar_id], //request to be passed
                department_thread_priority[depar_id],
                (const TaskHandle_t*)&department_SR_handles[depar_id])) == pdPASS)
            {
                printf("New SR!\r\n");
            }
        }
    }
}

TaskFunction_t ServiceRoutine(void* req) {
    request* reques = (request*)req;//
    department_id depar_id = reques->dep_id;
    SemaphoreHandle_t department_semaphore = semaphoreList[depar_id];

    int i = 0;
    while (i < reques->requested_vehicles) {

        if (xSemaphoreTake(globalSemaphore, 0) == pdPASS && xSemaphoreTake(semaphoreList[depar_id], 0) == pdPASS) {
            vTaskPrioritySet(NULL, uxTaskPriorityGet(NULL) + 1);// increases prioirity by one
            i++;
            totalVehicles--;
            department_list[POLICE].available_num--;
            department_list[POLICE].concurrent_num--;
            /* in case all resources for execution acquired, exectues, releases all semaphores and end task*/
            if (i == reques->requested_vehicles - 1) {
                vTaskDelay(reques->time_to_complete);
                releaseResources(i, depar_id);
                vTaskDelete(NULL);
            }
        }
        else {
            /* in deadlock releases all semaphores acquired and then start again*/
            releaseResources(i, depar_id);
            i = 0;


        }
    }
}



void releaseResources(int i, department_id depar_id) {
    for (int j = 0; j < i;j++) {
        xSemaphoreGive(globalSemaphore);
        xSemaphoreGive(semaphoreList[depar_id]);
        totalVehicles++;
        department_list[POLICE].concurrent_num++;
        department_list[POLICE].available_num++;
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
        TickType_t active_time = rand() % XTICKS_MAX;
        uint8_t vehicle_num_to_dispatch = rand() % MAX_NUM_REQUESTED_VEHICLES;
        request req = { dep,active_time,vehicle_num_to_dispatch };
        if ((retval_Send_To_Dispatch_Queue = xQueueSend(DispatchQueueHandle, &req, DISPATCH_QUEUE_TIMEOUT)) == pdPASS) {
            printf("request was put inside dispatch queue!\r\n");
        }
    }
}
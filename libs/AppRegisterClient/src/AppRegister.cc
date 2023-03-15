#include "AppRegisterClient/AppRegister.h"

#include <iostream>

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "AppRegisterCommon/AppRegister.h"
#include "AppRegisterCommon/Semaphore.h"

struct app_data* registerAttach(
    const char* app_name,
    long double requested_throughput,
    int max_threads,
    bool gpu_implementation) 
{
    pid_t controllerPid = getControllerPid(DEFAULT_CONTROLLER_NAME);

    /*--- begin app_data initialization ---*/
    //create shared memory to store application struct app_data structure
    int dataSegmentId = shmget(getpid(), sizeof(struct app_data), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    struct app_data* data = (struct app_data*) shmat(dataSegmentId, 0, 0);
    if (dataSegmentId == -1 || data == (void*)-1){
      std::cerr << "ERROR: While initilizing shared memory" << std::endl;
      exit(EXIT_FAILURE); 
    }

    //initialize struct app_data structure
    data->segment_id = dataSegmentId;
    data->use_gpu = 0;
    data->u_sleep_time = 0;
    data->precision_level = 0;
    data->n_cpu_cores = 1;
    data->requested_throughput = requested_throughput;

    //first entry contains 0 ticks at time zero (from application start!)
    data->ticks[0].value = 0;
    data->ticks[0].time  = 0;
    data->curr_tick = 0; 
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    data->start_time = tv.tv_sec + 0.000001*tv.tv_usec;
    data->lastTimeSample = data->start_time;
    /*--- end data initialization ---*/

    /*--- begin app_register initialization ---*/
    //lock
    int semId = semget (controllerPid, 1, 0);
    binarySemaphoreWait(semId);

    //attach to app_register in shared memory
    int registerSegmentId = shmget(controllerPid, sizeof(struct app_register), 0);
    struct app_register* appRegister = (struct app_register*) shmat (registerSegmentId, 0, 0);
    if (registerSegmentId == -1 || semId == -1 || appRegister == (void*)-1){
        std::cerr << "ERROR: While attaching to app_register in shared memory" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    //save application data in the app_register
    if(appRegister->n_new == MAX_NUM_OF_APPS){
      std::cerr << "ERROR: Maximum number of new applications reached" << std::endl;
      exit (EXIT_FAILURE);
    }

    appRegister->new_apps[appRegister->n_new].segment_id  = dataSegmentId;
    appRegister->new_apps[appRegister->n_new].pid         = getpid();
    appRegister->new_apps[appRegister->n_new].max_threads  = max_threads;
    strcpy(appRegister->new_apps[appRegister->n_new].name, app_name);
    appRegister->n_new++;

    //unlock
    binarySemaphorePost(semId);
    /*--- end app_register initialization ---*/

    return data;
}

int registerDetach(struct app_data* data) 
{ 
    pid_t controllerPid = getControllerPid(DEFAULT_CONTROLLER_NAME);
    //lock  
    int semId = semget (controllerPid, 1, 0);
    binarySemaphoreWait(semId);
    int registerSegmentId = shmget(controllerPid, sizeof(struct app_register), 0);
    struct app_register* appRegister = (struct app_register*) shmat (registerSegmentId, 0, 0);
    if (registerSegmentId == -1 || semId == -1 || appRegister == (void*)-1) {
        std::cerr << "Something wrong happened during controller shared memory attaching" << std::endl;
        exit(EXIT_FAILURE);
    }
    if(appRegister->n_detached == MAX_NUM_OF_APPS) {
        std::cerr << "ERROR: Maximum number of detached applications reached" << std::endl;
        exit (EXIT_FAILURE);
    }
    appRegister->detached_apps[appRegister->n_detached] = getpid();
    appRegister->n_detached++;
    //unlock
    binarySemaphorePost(semId);

    //delete shared memory for the application struct app_data structure
    int segmentId = data->segment_id;
    int errorDt = shmdt(data);
    if(errorDt == -1){
        std::cerr << "ERROR: While detaching from shared memory" << std::endl;
        exit (EXIT_FAILURE);
    }

    return 0;
}

void addTick(struct app_data *data, int value) 
{
    int next = (data->curr_tick + 1) % MAX_TICKS_SIZE; 
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    data->ticks[next].value = data->ticks[data->curr_tick].value + value;
    data->ticks[next].time = (tv.tv_sec + 0.000001*tv.tv_usec) - data->start_time;
    data->curr_tick = next;
}

pid_t getControllerPid(const char* controller_name) 
{
    pid_t controller_pid;
    std::string pipe = std::string("pgrep ") + controller_name;
    FILE *fp;
    fp = popen(pipe.c_str(), "r");
    if (fp == NULL){
        std::cerr << "ERROR: While reading controller pid" << std::endl;
        exit (EXIT_FAILURE); 
    }
    int ret = fscanf(fp, "%d", &controller_pid);
    if (feof(fp)){
        std::cerr << "ERROR: While reading controller pid" << std::endl;
        exit (EXIT_FAILURE);  
    }
    pclose(fp);
    return controller_pid;
}

void setRequestedThroughput(struct app_data* data, long double requested_throughput)
{
    data->requested_throughput = requested_throughput;
}

bool getUseGpu(struct app_data *data) 
{
    return data->use_gpu;
}

int getPrecisionLevel(struct app_data *data) 
{
    return data->precision_level;
}

int getUSleepTime(struct app_data* data) 
{
    return data->u_sleep_time;
}

int getNCpuCores(struct app_data *data) 
{
    return data->n_cpu_cores;
}

void autosleep(struct app_data* data, long double ref_thr) {
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    long double curr_time = tv.tv_sec + 0.000001*tv.tv_usec;
    data->u_sleep_time = (1.0/ref_thr - (curr_time - data->lastTimeSample))*1000000;

    if(data->u_sleep_time>0)
        usleep(data->u_sleep_time);
    else
        data->u_sleep_time = 0;
    gettimeofday(&tv, &tz);
    data->lastTimeSample = tv.tv_sec + 0.000001*tv.tv_usec;  
}

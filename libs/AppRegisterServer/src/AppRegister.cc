#include "AppRegisterServer/AppRegister.h"
#include "AppRegisterServer/CGroupUtils.h"

#include <fstream>
#include <iostream>
#include <queue>

#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/time.h>

#include "AppRegisterCommon/AppRegister.h"
#include "AppRegisterCommon/Semaphore.h"

namespace AppRegister 
{

    struct app_register* registerCreate(int numOfCores)
    {
        pid_t controller_pid = getpid();

        // Initialized app_register and its companion semaphore.
        // the semaphore is used for mutually exclusive access of the register 
        // between applications and controller
        int semid = semget(controller_pid, 1, IPC_CREAT | IPC_EXCL | 0666);
        union semun argument;
        unsigned short values[1] = {1};
        argument.array = values;
        int semval = semctl(semid, 0, SETALL, argument);

        int shmid = shmget(controller_pid, sizeof(struct app_register), IPC_CREAT | IPC_EXCL | 0666);
        struct app_register* app_register = (struct app_register*) shmat(shmid, 0, 0);
        if(semid == -1 || semval == -1 || shmid == -1 || app_register==(void*)-1){
            std::cerr << "ERROR: While initializing app_register in shared memory" << std::endl;
            exit(EXIT_FAILURE);
        }
        //

        // Initialize counters of applications
        app_register->n_new = 0;
        app_register->n_detached = 0;
        //

        // Setup CGroups
        int error = CGroupUtils::Setup(numOfCores);
        if(error==-1){
            std::cerr << "ERROR: While setting up CGroup" << std::endl;
            exit(EXIT_FAILURE);
        }
        //

        // Map the controller on the first CPU cluster
        error = CGroupUtils::Initialize(controller_pid);
        if(error==-1){
            std::cerr << "ERROR: While initializing CGroups" << std::endl;
            exit(EXIT_FAILURE);
        }
        std::vector<int> cpu0({0});
        error = CGroupUtils::UpdateCpuSet(controller_pid, cpu0);
        if(error==-1){
            std::cerr << "ERROR: While updating CGroups sets" << std::endl;
            exit(EXIT_FAILURE);
        }

        return app_register;
    }

    void registerDestroy(struct app_register* app_register) 
    {
        pid_t controller_pid = getpid();
    
        // Detach and delete app_register from shared memory
        int shmid = shmget(controller_pid, sizeof(struct app_register*), 0);
        int err = shmctl(shmid, IPC_RMID, 0);
        shmdt(app_register);
        if(shmid == -1 || err == -1){
            std::cerr << "ERROR: While deallocating app_register from shared memory" << std::endl;
            exit(EXIT_FAILURE);
        }
        int semid = semget(controller_pid, 1, 0);
        union semun ignored_argument;
        err = semctl(semid, 1, IPC_RMID, ignored_argument);
        if(semid == -1 || err == -1){
            std::cerr << "ERROR: While destroying app_register's semaphore" << std::endl;
            exit(EXIT_FAILURE);
        }
        //

        // Remove controller from CGroups
        int error = CGroupUtils::Remove(controller_pid);
        if(error==-1){
            std::cerr << "ERROR: While removing controller from CGroups" << std::endl;
            exit(EXIT_FAILURE);
        }
        // Destroy CGroups
        error = CGroupUtils::Destroy();
        if(error==-1){
            std::cerr << "ERROR: While destroying CGroups" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    struct app_data* getAppData(int segment_id)
    {
        struct app_data* shm = (struct app_data*) shmat(segment_id, 0, 0);
        if(shm == (void*) -1)
            std::cerr << "ERROR: While accessing app_data in shared memory!" << std::endl;
        return shm;
    }

    double getGlobalThroughput(struct app_data *data) 
    {
        // No ticks
        if(data->ticks[data->curr_tick].time == 0)
            return 0;

        // Get app's uptime
        struct timeval tv;
        struct timezone tz;
        gettimeofday(&tv, &tz);
        long double curr_time = (tv.tv_sec + 0.000001*tv.tv_usec);
        long double curr_uptime = curr_time - data->start_time;

        return data->ticks[data->curr_tick].value / curr_uptime;
    }

    double getCurrThroughput(struct app_data *data, int window) 
    {
        // No ticks
        if(data->ticks[data->curr_tick].time == 0)
          return 0;

        // Get app's uptime
        struct timeval tv;
        struct timezone tz;
        gettimeofday(&tv, &tz);
        long double curr_time = (tv.tv_sec + 0.000001*tv.tv_usec);
        long double curr_uptime = curr_time - data->start_time;

        // No ticks in the previous period
        if(curr_uptime - data->ticks[data->curr_tick].time > window)
            return 0;

        int prev_tick = (data->curr_tick==0) ? MAX_TICKS_SIZE-1 : data->curr_tick-1;
        while(curr_uptime - data->ticks[prev_tick].time <= window && !(prev_tick == 0 && data->ticks[prev_tick].time == 0) && prev_tick != data->curr_tick){
            if(prev_tick > 0) 
                prev_tick--;
            else
                prev_tick = MAX_TICKS_SIZE-1;
        }
        long double actual_window = window;
        if(prev_tick == 0 && data->ticks[prev_tick].time == 0){
            actual_window = data->ticks[data->curr_tick].time;
        } else if(prev_tick == data->curr_tick){ 
            prev_tick = (prev_tick+1) % MAX_TICKS_SIZE;
            actual_window = data->ticks[data->curr_tick].time - data->ticks[prev_tick].time;
        }

        return (data->ticks[data->curr_tick].value - data->ticks[prev_tick].value) / actual_window;
    }

    double getRequestThroughput(struct app_data* data){ return data->requested_throughput; }
    void setUseGpu(struct app_data* data, bool value) { data->use_gpu = value; }
    void setPrecisionLevel(struct app_data* data, int value) { data->precision_level = value;}
    void setUSleepTime(struct app_data* data, int value) { data->u_sleep_time = value; }
    void setNCores(struct app_data* data, int value) { data->n_cpu_cores = value; }

    bool isAppRunning(pid_t pid) 
    {
        std::string cmdname = std::string("/proc/") + std::to_string(pid) + std::string("/status");
        FILE *fp;
        fp = fopen(cmdname.c_str(), "r");
        if (fp){
            fclose(fp);
            return true;
        }
        return false;
    }

    std::vector<pid_t> getAppPids(pid_t pid) 
    {
        std::vector<pid_t> pids;
        std::string pipe = std::string("ps -e -T | grep ")+ std::to_string(pid) + std::string(" | awk '{print $2}'");
        FILE *fp;
        fp = popen(pipe.c_str(), "r");
        if (fp == NULL){
            std::cerr << "ERROR: While reading app PIDs" << std::endl;
            exit (EXIT_FAILURE);
        }
        pid_t tmp_pid;
        fscanf(fp, "%d", &tmp_pid);
        while (!feof(fp)) {
            pids.push_back(tmp_pid);
            fscanf(fp, "%d", &tmp_pid);
        }
        pclose(fp);
        return pids;
    }

    void killApp(pid_t pid)
    {
        //Remove CGroup settings
        int error = CGroupUtils::Remove(pid);
        if (error==-1){
          std::cerr << "ERROR: While removing CGroups" << std::endl;
          exit(EXIT_FAILURE);
        }
        std::vector<pid_t> pids = getAppPids(pid);
        for (auto p : pids) {
            int error=CGroupUtils::Remove(p);
            if (error==-1){
                std::cerr << "ERROR: While removing CGroups" << std::endl;
                exit(EXIT_FAILURE);
            }      
        }
        //Kill process
        kill(pid, SIGKILL);
    }

    void UpdateCpuSet(pid_t pid, std::vector<int> cores) 
    {
        int error=CGroupUtils::UpdateCpuSet(pid, cores);
        if (error==-1){
            std::cerr << "ERR0R: While updating CGroups" << std::endl;
            exit(EXIT_FAILURE);
        }   
    }

    void UpdateCpuQuota(pid_t pid, float quota) 
    {
        int error = CGroupUtils::UpdateCpuQuota(pid, quota);  
        if(error==-1){
            std::cerr << "ERR0R: While updating CGroups" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}

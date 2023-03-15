#ifndef APP_REGISTER_SERVER_APP_REGISTER
#define APP_REGISTER_SERVER_APP_REGISTER

#include <vector>

#include <sys/types.h>

#include "AppRegisterCommon/AppRegister.h"

#define DEFAULT_TIME_WINDOW 1

namespace AppRegister
{
    //Initialize app_register
    struct app_register* registerCreate(int numOfCores);
    //Destroy app_register
    void registerDestroy(struct app_register* app_register);

    //Get pointer to app_data
    struct app_data* getAppData(int segment_id);
    //Get global throughput
    double getGlobalThroughput(struct app_data* data);
    //Get current throughput 
    double getCurrThroughput(struct app_data* data, int window = DEFAULT_TIME_WINDOW);
    //Get requested throughput
    double getRequestedThroughput(struct app_data* data);
    //Set use_gpu
    void setUseGpu(struct app_data* data, bool value);
    //Set precision_level
    void setPrecisionLevel(struct app_data* data, int value);
    //Set u_sleep_time
    void setUSleepTime(struct app_data* data, int value);
    //Set n_cpu_cores
    void setNCpuCores(struct app_data* data, int value);

    //Check if the given app is running
    bool isAppRunning(pid_t pid);
    //Get thread ids of the given app
    std::vector<pid_t> getAppPids(pid_t pid);
    //Kill the given app
    void killApp(pid_t pid);
    //Set CPU cores for the given app
    void UpdateCpuSet(pid_t pid, std::vector<int> cores);
    //Set CPU quotas for the given app
    void UpdateCpuQuota(pid_t pid, float quota);
}

#endif //APP_REGISTER_CONTROLLER_APP_REGISTER

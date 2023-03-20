#ifndef APP_REGISTER_SERVER_APP_DATA
#define APP_REGISTER_SERVER_APP_DATA

#include "AppRegisterCommon/AppRegister.h"

#define DEFAULT_TIME_WINDOW 1

namespace AppData
{
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
}

#endif //APP_REGISTER_CONTROLLER_APP_DATA
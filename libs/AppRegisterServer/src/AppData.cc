#include "AppRegisterServer/AppData.h"

#include <iostream>

#include <sys/shm.h>
#include <sys/time.h>

#include "AppRegisterCommon/AppRegister.h"

namespace AppData 
{
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
}

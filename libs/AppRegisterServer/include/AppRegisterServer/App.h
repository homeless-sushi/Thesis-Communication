#ifndef APP_REGISTER_SERVER_APP
#define APP_REGISTER_SERVER_APP

#include <deque>

#include "AppRegisterServer/AppRegister.h"

#define TICKS_SLIDING_WINDOW_DEFAULT_SIZE 10

namespace App 
{
    class App
    {
        private:
            std::deque<struct ticks> ticksSlidingWindow;
            unsigned int ticksSWMaxSize;
            
        public:
            struct app_descriptor descriptor;
            struct app_data* data;

            App(app_descriptor descriptor,
                unsigned int ticksSWMaxSize = TICKS_SLIDING_WINDOW_DEFAULT_SIZE
            );
            ~App();

            void lock();
            void unlock();

            double getThroughput(unsigned int samplesWindow = 1);
    };
}

#endif //APP_REGISTER_SERVER_APP
#ifndef APP_REGISTER_SERVER_APP
#define APP_REGISTER_SERVER_APP

#include "AppRegisterServer/AppRegister.h"

namespace App 
{
    class App
    {
        public:
            struct app_descriptor descriptor;
            struct app_data* data;

            App(app_descriptor descriptor);
            ~App();

            void lock();
            void unlock();
    };
}

#endif //APP_REGISTER_SERVER_APP
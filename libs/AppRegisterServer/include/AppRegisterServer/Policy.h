#ifndef APP_REGISTER_SERVER_POLICY
#define APP_REGISTER_SERVER_POLICY

#include "AppRegisterServer/AppRegister.h"

#include <map>
#include <memory>
#include <vector>

#include <sys/types.h>

#include "AppRegisterCommon/AppRegister.h"

namespace Policy 
{
    class App
    {
        public:
            struct app_descriptor descriptor;
            struct app_data* data;

            App(app_descriptor descriptor);
            ~App();
    };

    class Policy 
    {
        protected:
            struct app_register* appRegister;
            std::map<pid_t, std::unique_ptr<App>> registeredApps;

            std::vector<pid_t> deregisterDeadApps();
            std::vector<pid_t> deregisterDetachedApps();
            std::vector<pid_t> registerNewApps();
            
        public:
            Policy(unsigned int nCores);
            virtual ~Policy();
            virtual void run(int cycle) = 0;
    };
}

#endif //APP_REGISTER_SERVER_POLICY

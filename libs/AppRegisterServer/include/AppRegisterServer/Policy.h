#ifndef APP_REGISTER_SERVER_POLICY
#define APP_REGISTER_SERVER_POLICY

#include "AppRegisterServer/AppRegister.h"
#include "AppRegisterServer/App.h"

#include <map>
#include <memory>
#include <vector>

#include <sys/types.h>

#include "AppRegisterCommon/AppRegister.h"

namespace Policy 
{
    class Policy 
    {
        protected:
            struct app_register* appRegister;
            std::map<pid_t, std::unique_ptr<App::App>> registeredApps;

            std::vector<pid_t> deregisterDeadApps();
            std::vector<pid_t> deregisterDetachedApps();
            std::vector<pid_t> registerNewApps();
            
        public:
            Policy(unsigned int nCores);
            virtual ~Policy();
            virtual void run(int cycle) = 0;

            void lock();
            void unlock();
    };
}

#endif //APP_REGISTER_SERVER_POLICY

#include "AppRegisterServer/AppRegister.h"
#include "AppRegisterServer/Policy.h"
#include "AppRegisterServer/CGroupUtils.h"

#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include <sys/shm.h>
#include <sys/types.h>

#include "AppRegisterCommon/AppRegister.h"
#include "AppRegisterCommon/Semaphore.h"

namespace Policy 
{
    App::App(app_descriptor descriptor) :
        descriptor(descriptor)
    {
        data = (struct app_data*) shmat(descriptor.segment_id, 0, 0);
        if(data == (void*) -1){
            std::cerr << "ERROR: While attaching app_data in shared memory!" << std::endl;
        }

        int error = CGroupUtils::Initialize(descriptor.pid);
        if(error==-1){
            std::cerr << "ERROR: While initializing CGroups" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    App::~App()
    {
        int errCtl = shmctl(descriptor.segment_id, IPC_RMID, 0);
        int errDt = shmdt(data); 
        if(errCtl == -1 || errDt == -1){
            std::cerr << "ERROR: While detaching app_data from shared memory" << std::endl;
        }

        int error=CGroupUtils::Remove(descriptor.pid);
        if (error==-1){
            std::cerr << "ERROR: While removing CGroups" << std::endl;
            exit(EXIT_FAILURE);
        } 
    }

    Policy::Policy(unsigned int nCores)
    {
        appRegister = AppRegister::registerCreate(nCores);
    }

    Policy::~Policy()
    {
        AppRegister::registerDestroy(appRegister);
    }

    std::vector<pid_t> Policy::deregisterDeadApps()
    {
        std::vector<pid_t> deadApps;
        for(auto i = registeredApps.cbegin(), next_i = i; i != registeredApps.cend(); i = next_i){
            ++next_i;
            pid_t pid = i->first;
            if(AppRegister::isAppRunning(pid)){
                continue;
            }
            registeredApps.erase(i);
            deadApps.push_back(pid);
        }
        return deadApps;
    }

    std::vector<pid_t> Policy::deregisterDetachedApps()
    {
        //lock
        pid_t controllerId = getpid();
        int semId = semget (controllerId, 1, 0);
        binarySemaphoreWait(semId);
        
        //deregister detached apps
        std::vector<pid_t> deregisteredApps;
        for(int i = 0; i < appRegister->n_detached; i++){
            pid_t detached_pid = appRegister->detached_apps[i];
            registeredApps.erase(detached_pid);
            deregisteredApps.push_back(detached_pid);
        }
        appRegister->n_detached = 0;

        //unlock
        binarySemaphorePost(semId);

        return deregisteredApps;
    }

    std::vector<pid_t> Policy::registerNewApps()
    {
        //lock
        pid_t controllerId = getpid();
        int semId = semget (controllerId, 1, 0);
        binarySemaphoreWait(semId);

        //register new apps
        std::vector<pid_t> newApps;
        for(int i = 0; i < appRegister->n_new; i++){
            struct app_descriptor descriptor = appRegister->new_apps[i];
            registeredApps[descriptor.pid] = std::unique_ptr<App>(new App(descriptor));
            newApps.push_back(descriptor.pid);
        }
        appRegister->n_new = 0;

        //unlock
        binarySemaphorePost(semId);

        return newApps;
    }
}

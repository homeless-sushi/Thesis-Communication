#include "AppRegisterServer/App.h"
#include "AppRegisterServer/AppUtils.h"
#include "AppRegisterServer/CGroupUtils.h"

#include <iostream>

#include <sys/shm.h>
#include <sys/types.h>

#include "AppRegisterCommon/AppRegister.h"
#include "AppRegisterCommon/Semaphore.h"

namespace App 
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

        if (AppUtils::isAppRunning(descriptor.pid))
            AppUtils::killApp(descriptor.pid);

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
}

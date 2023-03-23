#include "AppRegisterServer/AppRegister.h"
#include "AppRegisterServer/CGroupUtils.h"

#include <fstream>
#include <iostream>
#include <queue>
#include <vector>

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

        return app_register;
    }

    std::vector<pid_t> registerDestroy(struct app_register* app_register) 
    {
        pid_t controller_pid = getpid();
        int semid = semget (controller_pid, 1, 0);

        // Get the newly registered apps
        std::vector<pid_t> new_apps(app_register->n_new);
        for(int i = 0; app_register->n_new; i++){
            new_apps.push_back(app_register->new_apps[i].pid);
        }
    
        // Detach and delete app_register from shared memory
        int shmid = shmget(controller_pid, sizeof(struct app_register*), 0);
        int err = shmctl(shmid, IPC_RMID, 0);
        shmdt(app_register);
        if(shmid == -1 || err == -1){
            std::cerr << "ERROR: While deallocating app_register from shared memory" << std::endl;
            exit(EXIT_FAILURE);
        }

        union semun ignored_argument;
        err = semctl(semid, 1, IPC_RMID, ignored_argument);
        if(semid == -1 || err == -1){
            std::cerr << "ERROR: While destroying app_register's semaphore" << std::endl;
            exit(EXIT_FAILURE);
        }

        return new_apps;
    }
}

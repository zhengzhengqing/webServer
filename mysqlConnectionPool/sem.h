#ifndef SEM_H_
#define SWM_H_

#include <iostream>
#include <stdio.h>
#include <semaphore.h>
#include <exception>
using namespace std;

class Sem
{
    public:

        enum SEMSTATE
        {
            UnInit,
            Inited
        };

        Sem():semInit(UnInit)
        {}

        void init(int value)
        {
            if(sem_init(&sem,0, value) == 0)
            {
                semInit = Inited;
            }
            else
            {
                throw std::exception();
            }
        }

        ~Sem()
        {
            if(semInit == Inited)
                sem_destroy(&sem);
        }

        bool wait()
        {
            return sem_wait(&sem) == 0;
        }

        bool post()
        {
            return sem_post(&sem) == 0;
        }
        
    private:
        sem_t sem;
        SEMSTATE semInit;
};
#endif

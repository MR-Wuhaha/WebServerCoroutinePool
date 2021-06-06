#ifndef __Coroutine_Pool_H__
#define __Coroutine_Pool_H__
#include<vector>
#include<queue>
#include"libco/co_routine.h"
#include"Task.h"
#include"channel.h"
class CoroutinePool
{
    public:
        CoroutinePool(int _coroutine_size);
        void Start();
        void Run();
        void PutTask(const SP_channel& channel);
        static void* CoroutineFun(void *arg);
        static int CheckFinish(void *arg);
        ~CoroutinePool();
    private:
        int co_num;
        int free_co_num;
        std::vector<stCoRoutine_t*> coroutines;
        queue<SP_channel> task;
        stCoCond_t *cond;
};
#endif
#include"CoroutinePool.h"
CoroutinePool::CoroutinePool(int _coroutine_size):co_num(_coroutine_size),free_co_num(0)
{
    co_get_epoll_ct();
    cond = co_cond_alloc();
    coroutines.reserve(co_num);
}

void CoroutinePool::Start()
{
    for(int i = 0; i < co_num ; ++i)
    {
        stCoRoutine_t* co;
        co_create(&co, NULL, &CoroutinePool::CoroutineFun, this);
        coroutines.push_back(co);
        co_resume(co);
    }
}

void CoroutinePool::PutTask(const SP_channel& channel)
{
    task.push(channel);
}


//协程co_epoll的时候判断所有协程任务执行完时结束co_epoll循环，回到主协程继续监听
int CoroutinePool::CheckFinish(void* arg)
{
    CoroutinePool* mCoroutinePool = (CoroutinePool*)(arg);
    if(mCoroutinePool->free_co_num != mCoroutinePool->co_num)
    {
        /*
        printf("There is coroutine not finish, free_co_num %d != co_num %d\n", 
                mCoroutinePool->free_co_num, mCoroutinePool->co_num);
        */
        return 0;
    }
    else
    {
        //printf("All coroutine finish\n");
        return -1;
    }
}

void* CoroutinePool::CoroutineFun(void* arg)
{
    //通过指针获得协程池对象
    CoroutinePool* mCoroutinePool = (CoroutinePool*)(arg);
    uint32_t coroutine_id = mCoroutinePool->coroutines.size();
    while(true)
    {
        if(mCoroutinePool->task.size() == 0)
        {
            //没有任务时，协程才空闲，让出协程
            (mCoroutinePool->free_co_num)++;
            co_cond_timedwait(mCoroutinePool->cond, -1);
            continue;
        }
        //协程被唤醒时去取任务
        SP_channel channel = mCoroutinePool->task.front();
        mCoroutinePool->task.pop();
        channel->handle_event();
    }
}

void CoroutinePool::Run()
{
    //开启hook，hook的时候是hook住处理event的阻塞事件
    co_enable_hook_sys();
    uint32_t task_size = task.size();
    //如果任务为空，则一直循环
    while(task_size > 0)
    {
        //如果有协程是空闲的，并且任务没有被拿完，则唤醒一个协程来处理
        while(free_co_num > 0 && task_size > 0)
        {
            //printf("wake up a coroutine\n");
            co_cond_signal(cond);
            //空闲的协程减一
            free_co_num--;
            task_size--;
        }
        //如果空闲协程数小于总数，说明有协程正在运行，执行co_eventloop等所有协程执行完成
        if(free_co_num < co_num)
        {
            //co_eventloop传入CheckFinish函数，在每次循环最后判断是否还有协程在等待执行，如果没有就退出循环
            co_eventloop(co_get_epoll_ct(), CoroutinePool::CheckFinish, this);
        }
    }
    //解除hook
    co_disable_hook_sys();
}

CoroutinePool::~CoroutinePool()
{
    delete cond;
    for(int i = 0; i < coroutines.size(); ++i)
    {
        delete coroutines[i];
    }
}
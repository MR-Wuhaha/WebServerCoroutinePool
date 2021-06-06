#ifndef _EVENTLOOP
#define _EVENTLOOP
#include<iostream>
#include<vector>
#include"ThreadPool.h"
#include"Epoll.h"
#include"TimeRound.h"
#include"CoroutinePool.h"
class EventLoop
{
    public:
        EventLoop(int _epoll_size, int _coroutine_num = 0);
        Epoll* GetEpoll();
        static void* loop(void*);
        static void* MainLoop(void*);
    private:
        Epoll mEpoll;//当前线程的epoll
        int coroutine_num;//协程的数量
};

#endif

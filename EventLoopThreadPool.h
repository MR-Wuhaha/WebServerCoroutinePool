#ifndef _EVENT_LOOP_THREAD_POOL_H_
#define _EVENT_LOOP_THREAD_POOL_H_
#include<sys/eventfd.h>
#include<vector>
#include<fcntl.h>
#include<iostream>
#include<assert.h>
#include<pthread.h>
#include"libco/co_routine.h"
#include"Eventloop.h"
#include"TimeRound.h"
using namespace std;
class EventLoopThreadPool
{
    public:
        static TimeRound<channel>* time_round;
        EventLoopThreadPool(int _thread_num, int _coroutine_num);
        Epoll* GetNextEventEpollFd();//轮询获得下一个分配的event_fd
        void start();
        ~EventLoopThreadPool();
    private:
        vector<Epoll*> Epolls;//用于通知对应的epoll来了新的连接，此时会将唤醒的数据写到里面
        int thread_num;//线程的数量
        int coroutine_num;//协程的数量
        int cur_event_epoll;
};
#endif
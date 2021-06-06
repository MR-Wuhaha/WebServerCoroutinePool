#include"EventLoopThreadPool.h"
#include"Httpdata.h"

EventLoopThreadPool::EventLoopThreadPool(int _thread_num, int _coroutine_num) : thread_num(_thread_num),
cur_event_epoll(0),coroutine_num(_coroutine_num)
{
    Epolls.reserve(thread_num);
}

void EventLoopThreadPool::start()
{
    for(int i = 0;i < thread_num;i++)
    {
        EventLoop* event_loop = new EventLoop(100);
        Epoll* mepoll = event_loop->GetEpoll();
        if(mepoll == nullptr)
        {
            printf("Invalid Epollfd\n");
            assert(0);
        }
        int event_fd = eventfd(0,EFD_NONBLOCK);
        if(event_fd < 0)
        {
            printf("Invalid Eventfd\n");
            assert(0);
        }
        SP_channel event_channel = make_shared<Httpdata>(event_fd, EPOLLIN | EPOLLET, sysreadn, nullptr, time_round);
        mepoll->event_channel = event_channel;
        mepoll->Epoll_Add(event_channel, false);
        Epolls.push_back(mepoll);
        pthread_t pid;
        pthread_create(&pid, NULL, EventLoop::loop, event_loop);
    }
}

Epoll* EventLoopThreadPool::GetNextEventEpollFd()
{
    cur_event_epoll = (cur_event_epoll + 1) % thread_num;
    return Epolls[cur_event_epoll];
}

EventLoopThreadPool::~EventLoopThreadPool()
{

}

TimeRound<channel>* EventLoopThreadPool::time_round = nullptr;
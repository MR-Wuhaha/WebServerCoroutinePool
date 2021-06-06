#include"Eventloop.h"
#include"Httpdata.h"
using namespace std;

EventLoop::EventLoop(int _epoll_size, int _coroutine_num/*==0*/):mEpoll(_epoll_size),coroutine_num(_coroutine_num)
{

}

Epoll* EventLoop::GetEpoll()
{
    return &mEpoll;
}

/*
用于处理数据的线程Epoll，每个线程都先创建一个EventFd，当主线程收到新的连接时，通过向该EventFd发送数据
唤醒该线程中poll的协程，创建新的连接并将新的连接以协程的方式在该线程中运行
*/
void* EventLoop::loop(void* ptr)
{
    EventLoop* EVLoop = (EventLoop*)(ptr);
    //创建一个协程池，协程池主要处理的是连接接收到数据后，处理这部分请求的过程中，
    //进行协程切换的操作，与建立连接的socket没有关系，因为被唤醒了肯定是有数据可以读取的
    //而是在读取完数据处理的过程中，可能需要进一些rpc或者读取文件的操作，此时协程的切换
    //就有优势
    CoroutinePool mCoroutinePool(EVLoop->coroutine_num);
    mCoroutinePool.Start();
    cout<<"Thread Loop Start"<<endl;
    while(1)
    {
        vector<SP_channel> results = EVLoop->mEpoll.Poll();
        if(EVLoop->coroutine_num > 0)
        {
            for(int i = 0;i < results.size(); ++i)
            {
                mCoroutinePool.PutTask(results[i]);
            }
            //通过协程池处理连接的事件
            mCoroutinePool.Run();
        }
        else
        {
            for(int i = 0;i < results.size(); ++i)
            {
                results[i]->handle_event();
            }
        }
    }
}
//专门用于接收连接的epoll
void* EventLoop::MainLoop(void* ptr)
{
    EventLoop* EVLoop = (EventLoop*)ptr;
    CoroutinePool mCoroutinePoll(EVLoop->coroutine_num);
    mCoroutinePoll.Start();
    cout<<"Main Loop Thread Start"<<endl;
    while(true)
    {
        vector<SP_channel> event_channel_vec = EVLoop->mEpoll.Poll();
        if(EVLoop->coroutine_num > 0)
        {
            for(const auto& channel : event_channel_vec)
            {
                //channel->handle_event();
                mCoroutinePoll.PutTask(channel);
            }
            mCoroutinePoll.Run();
        }
        else
        {
            for(const auto& channel : event_channel_vec)
            {
                channel->handle_event();
            }
        }
    }
}

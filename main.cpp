#include<iostream>
#include"Epoll.h"
#include"channel.h"
#include"Eventloop.h"
#include"Task.h"
#include"ThreadPool.h"
#include"EventLoopThreadPool.h"
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<cstring>
#include"otherFun.h"
#include <functional>
#include"Httpdata.h"
#include"Log.h"
#include<unistd.h>
using namespace std;
int main(int argv,char** argc)
{
    int epoll_thread_num = 1;
    int coroutine_num = 0;
    if(argv >= 2)
    {
        epoll_thread_num = atoi(argc[1]);
    }
    if(argv >= 3)
    {
        coroutine_num = atoi(argc[2]);
    }
    if(epoll_thread_num < 0 || epoll_thread_num > 10 || coroutine_num > 50)
    {
        cerr<<"argment num error,enter command like './WebServer -threadnum -coroutinenum'"<<endl;
        cout<<"process quit..."<<endl;
        return 0;
    }
    else
    {
        printf("%d epoll thread, %d coroutine each thread create...\n",epoll_thread_num, coroutine_num);
    }
    //将当前进程变为守护进程在后台运行
    //int Defd = daemon(0,0);
    //assert(Defd == 0);


    //创建一个时间轮，参数为时间轮的最大定时时间间隔
    TimeRound<channel>* t;
#if 1
    TimeRound<channel> time_round = TimeRound<channel>(15);
    time_round.start();
    t = &time_round;
#endif
    //写日志对象
#if LOG_FLAG
    LogFile log_file("./ServerLog.txt");
    log_file.Start_Log();
    Log::log_file = &log_file;
#endif
    EventLoop *accept_epoll_loop = new EventLoop(100, coroutine_num);//创建请求连接的epoll
    int lsfd = socket(AF_INET,SOCK_STREAM,0);
    if(lsfd < 0)
    {
        cout<<"create listen socket fail"<<endl;
        assert(0);
    }
    struct sockaddr_in Server_addr;
    memset(&Server_addr,0,sizeof(Server_addr));
    Server_addr.sin_family = AF_INET;
    Server_addr.sin_port = htons(80);
    Server_addr.sin_addr.s_addr = htonl((in_addr_t)0);
    int reuse = 1;
    assert(set_noblock(lsfd) > 0);
    setsockopt(lsfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    int flag = bind(lsfd,(struct sockaddr*) &Server_addr,sizeof(Server_addr));
    if(flag < 0)
    {
        cout<<"bind listen socket fail"<<endl;
        assert(0);
    }
    listen(lsfd, 1000);
    SP_channel Listen(new Httpdata(lsfd, EPOLLIN | EPOLLET, Maccept, NULL, t));
    accept_epoll_loop->GetEpoll()->Epoll_Add(Listen, true);
    //如果只有一个线程，那么建立连接的epoll与处理实践的epoll都是同一个
    Listen->attach_epoll = accept_epoll_loop->GetEpoll();
    //如果参数线程多于1，则创建一个epoll的线程池，每个epoll创建一个协程池，把当前的线程作为创建连接的线程，其他线程处理请求
    EventLoopThreadPool event_loop_thread_pool(epoll_thread_num-1, coroutine_num);
    if(epoll_thread_num > 1)
    {
        Listen->SetEventLoopThreadPool(&event_loop_thread_pool);
        event_loop_thread_pool.time_round = t;
        event_loop_thread_pool.start();
    }
    EventLoop::MainLoop(accept_epoll_loop);
}

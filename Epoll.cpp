#include"Epoll.h"
using namespace std;

Epoll::Epoll(int _size):size(_size),event_fd(size)
{
    epfd = epoll_create(size);
    assert(epfd > 0);
}


int Epoll::Epoll_Add(SP_channel _channel,bool accept_flag)
{
    if(Mmap.count(_channel->fd) != 0)
    {
        cout<<"the fd is in the epoll list!,Add fail"<<endl;
        return -1;
    }
    Mmap[_channel->fd] = _channel;
    epoll_event epevent;
    if(accept_flag)
        _channel->accept_epoll = this;
    else
        _channel->attach_epoll = this;
    int flag = epoll_ctl(epfd,EPOLL_CTL_ADD,_channel->fd,&_channel->epevent);
    //cout<<"add fd "<< _channel->fd <<" success!"<<endl;
    return 0;
}

int Epoll::Epoll_Del(int fd)
{
    if(Mmap.count(fd) == 0)
    {
        cout<<"the fd is not in the epoll list,del fail"<<endl;
        return -1;
    }
    int flag = epoll_ctl(epfd,EPOLL_CTL_DEL,fd,NULL);
    Mmap.erase(fd);
    //cout<<"del fd "<< fd <<" success!"<<endl;
    return 0;
}

int Epoll::Epoll_Mod(SP_channel _channel)
{
    epoll_event epevent;
    int flag = epoll_ctl(epfd,EPOLL_CTL_MOD,_channel->fd,&_channel->epevent);
    //cout<<"mod fd "<< _channel->fd <<" success!"<<endl;
    return 0;
}

vector<SP_channel> Epoll::Poll()
{
    int event_count = epoll_wait(epfd,&*event_fd.begin(),event_fd.size(),-1);
    if(event_count > 0)
    {
        vector<SP_channel> result;
        for(int i = 0; i<event_count ;i++)
        {
            if(event_channel.get() != nullptr && event_fd[i].data.fd == event_channel->fd)
            {
                //是唤醒线程的信息，只读，不进行其他操作
                char wake_up_message[1024];
                read(event_channel->fd, wake_up_message, sizeof(wake_up_message));
                continue;
            }
            Mmap[event_fd[i].data.fd]->Revent = event_fd[i].events;
            Mmap[event_fd[i].data.fd]->epevent = event_fd[i];
            result.push_back(Mmap[event_fd[i].data.fd]);
        }
        return result;
    }
}
#ifndef _Epoll
#define _Epoll
#include<sys/epoll.h>
#include"channel.h"
#include<vector>
#include<assert.h>
#include<iostream>
#include<unordered_map>
using namespace std;
class Epoll
{
    private:
        int epfd;
        int size;
        unordered_map<int,SP_channel> Mmap;//用于快速获取产生事件的channel
    public:
        SP_channel event_channel;
        vector<epoll_event> event_fd;
        Epoll(int _size);
        int Epoll_Add(SP_channel,bool);
        int Epoll_Del(int fd);
        int Epoll_Mod(SP_channel);
        vector<SP_channel> Poll();
};

#endif
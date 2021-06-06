#include"otherFun.h"
#include"Epoll.h"
#include"channel.h"
#include"Log.h"
#include"EventLoopThreadPool.h"
using namespace std;
int co_accept(int fd, struct sockaddr *addr, socklen_t *len );
int Maccept(SP_channel _channel,char *buff,int length)
{
    while(true)
    {
        struct sockaddr_in Client_addr;
        memset(&Client_addr,0,sizeof(Client_addr));
        socklen_t Client_addr_len = sizeof(Client_addr);
        int connfd = accept(_channel->fd,(struct sockaddr*)&Client_addr,&Client_addr_len);
        if(connfd > 0)
        {
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET,&Client_addr.sin_addr.s_addr,client_ip,sizeof(client_ip));
#if LOG_FLAG
            LOG<<"accept connect from : " + string(client_ip) + " port:" + to_string(Client_addr.sin_port) << " ClientFd:" << connfd;
#endif
            //cout<<"accept connect from : "<<client_ip << " port:" << Client_addr.sin_port <<endl;
            /*
            如果只有一条线程在运行，则当前的线程作为处理连接数据和接收新连接的复用线程
            如果有多条线程在运行，则当前线程只用于建立连接，而将新建立的连接轮询分发到其他线程处理
            */
            if(_channel->event_loop_thread_pool == nullptr)
            {
                _channel->Add_New_Connect(connfd);
            }
            else
            {
                Epoll* event_epoll = _channel->event_loop_thread_pool->GetNextEventEpollFd();
                event_epoll->event_channel->Add_New_Connect(connfd);
                //唤醒被分配fd的线程
                uint32_t wake_up_message = 1;
                //write(event_epoll->event_channel->fd, &wake_up_message, sizeof(uint32_t));
            }
        }
        else
        {
            if(errno == EINTR)
            {
                continue;
            }
            else if(errno == EAGAIN)
            {
                break;
            }
        }
    }
    return -1;
}
int readn(SP_channel _channel,char *buff,int length)
{
    memset(buff,0,length);
    int read_len = 0;
    while(length > 0)
    {
        int rlen = recv(_channel->fd,buff,length,0);
        if(rlen < 0)
        {
            if(errno == EAGAIN)
            {
                break;
            }
            else if(errno == EINTR)
            {
                continue;
            }
        }
        else if(rlen > 0)
        {
            length = length - rlen;
            buff = buff + rlen;
            read_len += rlen;
        }
        else
        {
            //cout<<"client fd: "<<_channel->fd<<" closed or exception happend!"<<endl;
#if LOG_FLAG
            LOG<<"client fd: "+to_string(_channel->fd) + " closed or exception happend!";
#endif
            //对端关闭连接或者连接异常断开
            _channel->handle_close();
            return 0;
        }
    }
    return read_len;
}

int writen(SP_channel _channel,char *buff,int length)
{
    while(length > 0)
    {
        int wlen = send(_channel->fd,buff,length,0);
        if(wlen < 0)
        {
            if(errno == EAGAIN)
            {
                break;
            }
            else if(errno == EINTR)
            {
                continue;
            }
        }
        else if(wlen > 0)
        {
            length = length - wlen;
            buff = buff + wlen;
        }
        else
        {
            //cout<<"client fd: "<<_channel->fd<<" closed or exception happend!"<<endl;
#if LOG_FLAG
            LOG<<"client fd: "+to_string(_channel->fd) + " closed or exception happend!";
#endif
            //对端关闭连接或者连接异常断开
            _channel->handle_close();
            return 0;
        }
    }
    //cout<<"send to client:"<<_channel->fd<<" success!"<<endl;
    //delete (char*)_channel->epevent.data.ptr;
    //_channel->epevent.data.ptr = NULL;
    if(length == 0)
    {
        return -1;
    }
    return length;
}

//协程的读函数，协程由内部自己设置为noblock，所以使用这个读函数就可以了
int co_readn(SP_channel _channel,char *buff,int length)
{
    memset(buff,0,length);
    int rlen = recv(_channel->fd,buff,length,0);
    if(rlen == 0)
    {
        //cout<<"client fd: "<<_channel->fd<<" closed or exception happend!"<<endl;
#if LOG_FLAG
        LOG<<"client fd: "+to_string(_channel->fd) + " closed or exception happend!";
#endif        
        //对端关闭连接或者连接异常断开
        _channel->handle_close();
        return 0;
    }
    return rlen;
}

//协程的写函数
int co_writen(SP_channel _channel,char *buff,int length)
{
    int wlen = send(_channel->fd,buff,length,0);
    if(wlen == 0)
    {
        //cout<<"client fd: "<<_channel->fd<<" closed or exception happend!"<<endl;
#if LOG_FLAG
        LOG<<"client fd: "+to_string(_channel->fd) + " closed or exception happend!";
#endif        
        //对端关闭连接或者连接异常断开
        _channel->handle_close();
        return 0;
    }
    return wlen;
}

int set_noblock(int sockfd)
{
    if(sockfd < 0)
    {
        cout<<"sockfd is invailed"<<endl;
        return -1;
    }
    int flags = fcntl(sockfd,F_GETFL,0);
    fcntl(sockfd,F_SETFL,flags | O_NONBLOCK);
    return 1;
}

int set_block(int sockfd)
{
    if(sockfd < 0)
    {
        cout<<"sockfd is invailed"<<endl;
        return -1;
    }
    int flags = fcntl(sockfd,F_GETFL,0);
    flags &= ~O_NONBLOCK;
    fcntl(sockfd,F_SETFL,flags);
    return 1;
}
#include"channel.h"
#include"Epoll.h"
#include"Eventloop.h"
using namespace std;
channel::channel(int _fd,uint32_t _epevent, Handle _read, Handle _write, TimeRound<channel>* _time_round):
fd(_fd),write(_write),read(_read),time_round(_time_round),
event_loop_thread_pool(nullptr),event_loop(nullptr),Sevent(_epevent)
{
    wp_time_round_item = std::weak_ptr<TimeRoundItem<channel>>();
    max_read_write_buff_length = 4096;
    read_buff = new char[max_read_write_buff_length];
    write_buff = new char[max_read_write_buff_length];
    read_length = 0;
    write_length = 0;
    epevent.data.fd = _fd;
    epevent.events = Sevent;
}
int channel::handle_event()
{
    HandleRead();
    return 0;
}


void channel::Add_New_Connect(int fd)
{
    set_noblock(fd);
    SP_channel conn_socket(new channel(fd, EPOLLIN | EPOLLET, readn, writen, time_round));
    attach_epoll->Epoll_Add(conn_socket, false);
}

void channel::Close()
{
    attach_epoll->Epoll_Del(fd);
}

channel::~channel()
{
    close(fd);
#if LOG_FLAG
    LOG <<"client fd: "<<fd<<" has been closed";
#endif
    delete[] read_buff;
    delete[] write_buff;
}

int channel::get_fd()
{
    return fd;
}
void channel::SeparateTimer()
{
    std::shared_ptr<TimeRoundItem<channel>> temp;
    //获得当前fd对应的定时器，如果有的话，需要先分离，lock()是线程安全的
    if(temp = wp_time_round_item.lock())
    {
        temp->reset();//将绑定到该定时器上的fd分离
    }
}

void channel::HandleRead()
{
    read_length = read(shared_from_this(),read_buff,max_read_write_buff_length);
}

void channel::HandleWrite()
{
    int wlen = write(shared_from_this(),write_buff,write_length);
    if(wlen > 0 && wlen < write_length)
    {
        memcpy(write_buff,write_buff + (write_length - wlen),wlen);
        write_length = wlen;
        epevent.events = EPOLLOUT | EPOLLIN | EPOLLET;
        attach_epoll->Epoll_Mod(shared_from_this());
    }
    else
    {
        epevent.events = EPOLLIN | EPOLLET;
        attach_epoll->Epoll_Mod(shared_from_this());
    }
}

int channel::handle_close()
{
    //服务器端关闭连接
    std::shared_ptr<TimeRoundItem<channel>> temp;
    if(temp = wp_time_round_item.lock())
    {
        temp->reset();
    }
    Close();
}

void channel::SetEventLoopThreadPool(EventLoopThreadPool* _event_loop_thread_pool)
{
    this->event_loop_thread_pool = _event_loop_thread_pool;
}

bool channel::Get_KeepAlive_State()
{
    return false;
}

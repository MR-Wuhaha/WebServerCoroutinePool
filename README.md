基于腾讯协程库libco建立协程池的Web服务器
==============

## 主要原理
1、同一个线程采用协程池处理数据，减少协程的创建和析构的开销，特别是短连接时，如果一个链接创建一个协程，开销会非常巨大，因为处理完又要马上销毁<br>
2、如果运行的线程数为1，当前的线程会负责新连接的建立以及处理新连接的数据，如果定义的协程数为0，则不用协程池处理，建议线程数等于CPU的核心数<br>
3、如果运行的线程数大于1，则当前的线程仅作为创建连接的线程，将新建立的连接以协程的方式分配到其他的线程中<br>
4、采用EventFd的方式作为主线程通知其他线程新连接的fd<br>
5、协程的切换只会发生在数据处理的过程中，而不是socket接收数据的过程，数据处理时发生其他网络数据请求或者读取文件时会发生协程的切换<br>
6、EventLoop线程运行之后会创建一个协程池，有任务产生时会将任务添加到协程池中，运行协程池会触发协程池处理任务<br>
7、日志与线程池采用双缓冲队列实现写日志与线程任务的执行<br>
8、采用时间轮管理超时连接


## 代码使用方法
### 腾讯协程库
如果腾讯协程库还未编译，则按照以下的方式编译：<br>
cd libco/build<br>
make<br>
会生成一个协程动态链接库，作为后续项目链接使用

### 本项目代码
cd WebServerCoroutinePool/<br>
mkdir build<br>
cd build<br>
cmake ..<br>
make<br>

### 代码运行
./WebServerBaseCoroutinePool (int)thread_num (int)coroutine_num<br>
thread_num参数为线程的数量，不加该参数默认为1条线程，加上该参数则会运行thread_num条线程，其中主线程用于建立连接，其他线程处理连接的数据<br>
coroutine_num参数为协程的数量，默认为0
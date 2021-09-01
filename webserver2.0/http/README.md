## http连接处理类

根据状态转移,通过主从状态机封装了http连接类。其中,主状态机在内部调用从状态机,从状态机将处理状态和数据传给主状态机

* 客户端发出http连接请求
* 从状态机读取数据,更新自身状态和接收数据,传给主状态机
* 主状态机根据从状态机状态,更新自身状态,决定响应请求还是继续读取

#### 基础知识方面
* epoll、http报文格式、状态码和有限自动机
* 对服务器端处理http请求的全部流程进行简要介绍，然后结合代码对http类及请求接收进行详细分析

#### epoll
* epoll_create函数  创建一个指示epoll内核事件表的文件描述符，该描述符将用作其他epoll系统调用的第一个参数，size不起作用
  * int epoll_create(int size)
  * size没用
* epoll_ctl函数  该函数用于操作内核事件表监控的文件描述符上的事件：注册、修改、删除
  * int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
    * epfd：为epoll_creat的句柄
    * op：表示动作，用3个宏来表示：
      * EPOLL_CTL_ADD (注册新的fd到epfd)，
      * EPOLL_CTL_MOD (修改已经注册的fd的监听事件)，
      * EPOLL_CTL_DEL (从epfd删除一个fd)；
    * event：告诉内核需要监听的事件
* int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
  * events：用来存内核得到事件的集合
  * maxevents：告之内核这个events有多大，这个maxevents的值不能大于创建epoll_create()时的size
  * timeout：是超时时间
  * 返回值：成功返回有多少文件描述符就绪，时间到时返回0，出错返回-1
#### select、poll、epoll
#### ET、LT、EPOLLONESHOT
## HTTP报文格式
#### 请求报文
* HTTP请求报文由请求行（request line）、请求头部（header）、空行和请求数据四个部分组成。
其中，请求分为两种，GET和POST
* 报文格式

#### GET和POST

#### ET和LT
* ET下需要一次性将所有数据读完  边沿触发
* LT则不需要  水平触发

#### EPOLL
1. 设置非阻塞 fcntl（F_GETFl、F_SETFL）
2. 内核事件表注册新事件    内核事件表注册新事件，开启EPOLLONESHOT，针对客户端连接的描述符，listenfd不用开启
3. 内核事件表删除事件
4. 重置epolloneshot事件

#### 服务器接收http请求
* 浏览器端发出http连接请求，主线程创建http对象接收请求并将所有数据读入对应buffer，将该对象插入任务队列，工作线程从任务队列中取出一个任务进行处理。
* 各子线程通过process函数对任务进行处理，调用process_read函数和process_write函数分别完成报文解析与报文响应两个任务
* 在HTTP报文中，每一行的数据由\r\n作为结束字符，空行则是仅仅是字符\r\n。因此，可以通过查找\r\n将报文拆解成单独的行进行解析，项目中便是利用了这一点。

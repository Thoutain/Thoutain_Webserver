# select、poll、epoll、网络I/O

开源组件：

1. openssl
2. zeromq



网络I/O相关



为什么要有select、poll、epoll？

* I/O多路复用
* 较少的系统调用
* 没有的话怎么办？
  * 多开线程
  * 遍历所有fd 轮询recive
  * 两种方法都是代价比较大的
  * 用的都是recive这些东西
* 专门用一个组件来判别I/O里面有没有数据
  * 是否可读
  * 是否可写
  * 是否出错



五种网络I/O模型

* 阻塞IO
* 非阻塞IO
* 多路复用
* 信号驱动IO
* 异步IO



异步怎么理解？

* 检测是否有数据与读写数据不在同一个状态  不在同一个流程



信号IO

* 太频繁的信号调用影响性能
* 从内核回调  消息多会存到内核 占内存
* 从系统态回调到用户态
* 一个信号回调一次



select、poll重点

* 检测IO是什么意思：为什么需要检测IO
* IO可读可写的状态是什么意思
* 两者类似的，只是poll把select的读写错误数组传到一起了



数据在哪里：

* 数据从网关过来之后，先在recvbuffer里面
* 放在内核
* socket  对应一个 Tcb控制块  里面有一个sendbuffer和recvbuffer
* 然后对应的往下面走就是网卡



epoll

* 链表＋红黑树
* epoll_create()  创建
  * int opfd = epoll_create(int num)  这个num只有大于零和小于零的区别  现在没意义了  以往是决定能收多少东西的
* epoll_ctl()  往epoll里面添加、删除
* epoll_wait() 扫描
* 红黑树里面的结点和队列里面的结点是共用的结点

```C++
int epfd = epoll_create(1);
struct epoll_event ev, events[EPOLL_SIZE] = {0};

ev.events = EPOLLIN;
ev.data.fd = sockfd;
epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev); // sockfd as key

while (1){
    int nready = epoll_wait(epfd, events, EPOLL_SIZE, -1);
    if (nready == -1)
        continue;
    
    int i = 0;
    for (int i = 0; i < nready; i++){
        if (events[i].data.fd = sockfd){
            struct sockaddr_in client_addr;
				memset(&client_addr, 0, sizeof(struct sockaddr_in));
				socklen_t client_len = sizeof(client_addr);
			
				int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
				if (clientfd <= 0) continue;
	
				char str[INET_ADDRSTRLEN] = {0};
				printf("recvived from %s at port %d, sockfd:%d, clientfd:%d\n", inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)),
					ntohs(client_addr.sin_port), sockfd, clientfd);

				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = clientfd;
				epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clientfd, &ev);
        } else {
            int clientfd = events[i].data.fd;

			char buffer[BUFFER_LENGTH] = {0};
			int ret = recv(clientfd, buffer, BUFFER_LENGTH, 0);
			if (ret < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				printf("read all data");
			}
					
			close(clientfd);
					
			ev.events = EPOLLIN | EPOLLET;
			ev.data.fd = clientfd;
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clientfd, &ev);
        } else if (ret == 0) {
			printf(" disconnect %d\n", clientfd);
					
			close(clientfd);

			ev.events = EPOLLIN | EPOLLET;
			ev.data.fd = clientfd;
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clientfd, &ev);
					
			break;
		} else {
			printf("Recv: %s, %d Bytes\n", buffer, ret);
		}
    }
}
```

ET & LT

* ET边沿触发：没有到有进行一次触发
* LT水平触发：有数据就触发，持续的读
* 传输大块数据的时候，建议用LT 数据大于recvbuffer 一次读不完
* 传输小块数据的时候，建议用ET  数据小于recvbuffer
* listenfd用LT水平触发，持续不断的触发，用ET的话有时候accept可能会漏掉一些东西



ET+状态机

* ET读
* 状态机模式处理业务



windows下用iocp


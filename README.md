# 基于Reactor的Web服务器-C++
## 1.模型框架

服务器分为两个版本，2.0为1.0的迭代，修改了1.0的部分不足，同时增加了一部分功能。红色部分为2.0新增加的功能。

​																																																---主要为了便于讲述

![image-20220208210650992](https://github.com/666dhl/MyWebServer/blob/main/images/www.yalijuda.com_4d78890f8f3229c3bc3c83b263b84f2_DUCG_NPBkY.png)

### 1.主线程

![image-20220208210823604](https://github.com/666dhl/MyWebServer/blob/main/images/www.yalijuda.com_6422267dcf559e49349d74b8d7df38a_5nSTo7JQ1R.png)

![image-20220209011236504](https://github.com/666dhl/MyWebServer/blob/main/images/www.yalijuda.com_ca491cf34b62d9c2592ec6b9562ba68_pjXRm931U2.png)

![image-20220209011207617](https://github.com/666dhl/MyWebServer/blob/main/images/www.yalijuda.com_0bc1e860b7fd799ca06b6de12e27925_ase6LqbAJD.png)

### 2.线程池

![image-20220208210919653](https://github.com/666dhl/MyWebServer/blob/main/images/www.yalijuda.com_16ffd2d834ee4f0f3089c12a2d11ef8_39NtSCv4VA.png)

![image-20220208211014624](https://github.com/666dhl/MyWebServer/blob/main/images/www.yalijuda.com_99d82fee92a209aae5fa81b8092c9c6_Kk0cF5nuzM.png)

### 3.requestdata结构体

![image-20220208210947849](https://github.com/666dhl/MyWebServer/blob/main/images/www.yalijuda.com_cc32028a46237348beef01bd8d5c3ff_0rRJYeoxBZ.png)

![image-20220208211211346](https://github.com/666dhl/MyWebServer/blob/main/images/www.yalijuda.com_aa57ac4b89d59912ceb77242d1f5f1f_t5yNcJ0W7D.png)

## 2.详细注释
详细的源码讲解请看注释

## 3.特性

### 版本1.0

```
1. 在整个epoll监听循环开始之前  先屏蔽掉SIGPIPE信号
	//默认读写一个关闭的socket会触发sigpipe信号 该信号的默认操作是关闭进程 这明显是我们不想要的
    //所以我们需要重新设置sigpipe的信号回调操作函数   比如忽略操作等  使得我们可以防止调用它的默认操作 
    //信号的处理是异步操作  也就是说 在这一条语句以后继续往下执行中如果碰到信号依旧会调用信号的回调处理函数处理sigpipe信号
void handle_for_sigpipe()
{
    struct sigaction sa; //信号处理结构体
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;//设置信号的处理回调函数 这个SIG_IGN宏代表的操作就是忽略该信号 
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))//将信号和信号的处理结构体绑定
        return;
}

```

```
2. epoll监管套接字的时候用边沿触发+EPOLLONESHOT+非阻塞IO   

```

```
3. 使用多线程充分利用多核CPU，并使用线程池避免线程频繁创建销毁的开销
创建一个线程池：线程池中主要包含任务队列和工作线程集合使用了一个固定线程数的工作线程
工作线程之间对任务队列的竞争采用条件变量和互斥锁结合使用
一个工作线程先加互斥锁，当任务队列中任务数量为0时候，pthread_cond_wait()阻塞条件变量,
当任务数量大于0时候，用pthread_cond_broadcast()条件变量通知阻塞在条件变量下的线程，线程来继续竞争获取任务
对任务队列中任务的调度采用先来先服务算法
```
```
4.采用reactor模式 主线程只负责IO  获取io请求后把请求对象给工作线程 工作线程负责数据读取以及逻辑处理
```

```
5. 在主线程循环监听到读写套接字有报文传过来以后 在工作线程调用requestData中的handleRequest进行使用状态机解析了HTTP请求
http报文解析和报文响应 解析过程状态机如上图所示. 
值得注意的是

(1)这里支持了两种类型GET和POST报文的解析 

根据http请求报文的请求行去判断是请求类型字符串是"GET"还是"POST"
然后用一个map存放首部行的键值对数据
如果是post报文的话,首部行里面必然会有Content-length字段而get没有所以取出这个字段 求出后面实体主体时候要取用的长度 
然后往下走回送相应的http响应报文即可
而get报文实体主体是空的，直接读取请求行的url数据，然后往下走回送相应的http响应报文即可

(2)在这是支持长连接 keep-alive

 在首部行读取出来数据以后如果请求方设置了长连接 则Connection字段为keep-alive以此作为依据
 如果读取到这个字段的话就在报文解析 报文回送完毕之后将requestData重置 
 然后将该套接字属性也用epoll_ctl重置 再次加入epoll监听

```
```
6.实现了一个小根堆的定时器及时剔除超时请求，使用了STL的优先队列来管理定时器
```
### 版本2.0

```
1.将线程池实现动态扩容机制--创建管理者线程（10s循环一次）
在线程初始化的时候创建管理者线程实现在高并发场景下，线程池容量的动态扩容，最大为100。在低并发场景下，线程池数量缩减至最低数量4。
扩容机制：
当忙线程数量>80%活着线程数，一次扩容10个线程
当忙线程数量<20%活着线程数，一次销毁10个线程
```

```
2.修改版本1.0，当任务队列满时，又有任务进入时，直接抛弃数据结构体的缺点。
增加任务部位满的条件变量，线程池添加任务时，判断任务队列是否满，如果满，则阻塞等待。
线程池工作线程每次取完任务，将唤醒条件变量。
```

```
3.增加对文件和目录的GET请求。
打开文件（html/c/doc/gif/mp3等等），获取文件描述符，通过建立映射区，建立文件与内存的映射，发送给客户端-如下图
目录会将目录文件下的文件名称以及类型，发送给客户端。
```

![image-20220208215814204](https://github.com/666dhl/MyWebServer/blob/main/images/www.yalijuda.com_439b896611197f357810174487339df_N1ML_2Un0m.png)

![image-20220208215829582](https://github.com/666dhl/MyWebServer/blob/main/images/www.yalijuda.com_ac199901b782985c3d994ecb0cee98b_zmt05TwCiJ.png)

## 4.编译及测试

```
编译过程：
1.git clone https://github.com/666dhl/MyWebServer.git
2.cd MyWebServer/version_1.0/
3.mkdir build
4.cd build
5.cmake ..&&make
6.cd ../bin
7../MyWebServer 
测试过程
127.0.0.1:8888              获取的为默认html文件
127.0.0.1:8888/文件或目录     获取目录或文件信息
```

## 5.性能

用webbench压测工具 
并发1000个请求 压测10s 结果如下

### 版本1.0

![image-20220208221259764](https://github.com/666dhl/MyWebServer/blob/main/images/www.yalijuda.com_899485d6ac3cb3c77053eb82e30d848_OFR3bzkoj9.png)

### 版本2.0

![image-20220208222308146](https://github.com/666dhl/MyWebServer/blob/main/images/www.yalijuda.com_7f4ff9c93e5ace08f998866b57c28da_HD6IkhlSE7.png)

明显看出修改后的版本，性能更强。

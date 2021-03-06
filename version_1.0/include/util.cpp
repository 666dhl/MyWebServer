#include "util.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

ssize_t readn(int fd, void *buff, size_t n) {
    // 从文件描述符fd中读取n个字节到buff数组中
    size_t nleft = n;// 总共需要读的字节数
    ssize_t nread = 0;// 每次调用read读到的字节数
    ssize_t readSum = 0;// 通过read函数总共读到的字节数
    char *ptr = (char*)buff;// 写到buff数组中ptr指定的位置
    while (nleft > 0) {
        if ((nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR) 
                nread = 0;
            else if (errno == EAGAIN) {
                // 非阻塞地读，缓冲区此时已空，可以直接返回
                return readSum;
            } else {
                // 错误
                return -1;
            }  
        } else if (nread == 0) {

            break;
        }
        readSum += nread;
        nleft -= nread;
        ptr += nread;
    }
    return readSum;
}

ssize_t writen(int fd, void *buff, size_t n) {
    size_t nleft = n;
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    char *ptr = (char*)buff;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0) {
            if (nwritten < 0) {
                if (errno == EINTR || errno == EAGAIN) {
                    nwritten = 0;
                    continue;
                }
                else
                    return -1;
            }
        }
        writeSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    return writeSum;
}

//处理sigpipe信号
void handle_for_sigpipe() {
    struct sigaction sa; //信号处理结构体
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN; //设置信号的处理回调函数 这个SIG_IGN宏代表的操作就是忽略该信号 
    sa.sa_flags = 0;

    //将信号和信号的处理结构体绑定
    if(sigaction(SIGPIPE, &sa, NULL)) return;
}

int setSocketNonBlocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    
    if(flag == -1) return -1;

    flag |= O_NONBLOCK;

    if(fcntl(fd, F_SETFL, flag) == -1) return -1;
    
    return 0;
}
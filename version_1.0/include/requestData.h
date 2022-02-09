#ifndef REQUESTDATA
#define REQUESTDATA
#include <string>
#include <cstring>
#include <unordered_map>
#include "requestData.h"
#include "util.h"
#include "epoll.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/time.h>
#include <unordered_map>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <queue>
#include <iostream>
using namespace std;

// 状态机的各个状态
const int STATE_PARSE_URI = 1;// 解析url
const int STATE_PARSE_HEADERS = 2;// 解析http头部
const int STATE_RECV_BODY = 3;// 接收http报文主体
const int STATE_ANALYSIS = 4;// 分析
const int STATE_FINISH = 5;// 结束
// 最大缓冲buffer大小
const int MAX_BUFF = 4096;

// 有请求出现但是读不到数据,可能是Request Aborted,
// 或者来自网络的数据没有达到等原因,
// 对这样的请求尝试超过一定的次数就抛弃
const int AGAIN_MAX_TIMES = 200;

const int PARSE_URI_AGAIN = -1;
const int PARSE_URI_ERROR = -2;
const int PARSE_URI_SUCCESS = 0;

const int PARSE_HEADER_AGAIN = -1;
const int PARSE_HEADER_ERROR = -2;
const int PARSE_HEADER_SUCCESS = 0;

const int ANALYSIS_ERROR = -2;
const int ANALYSIS_SUCCESS = 0;

const int METHOD_POST = 1;
const int METHOD_GET = 2;
const int HTTP_10 = 1;
const int HTTP_11 = 2;

const int EPOLL_WAIT_TIME = 500;






class MimeType {
private:
    static pthread_mutex_t lock;
    static std::unordered_map<std::string, std::string> mime;
    MimeType();
    MimeType(const MimeType &m);
public:
    static std::string getMime(const std::string &suffix);
};

enum HeadersState {
    h_start = 0,
    h_key,// 键
    h_colon,// 冒号
    h_spaces_after_colon,// 冒号后的空格
    h_value,// 值
    h_CR,// 非最后一行的\r
    h_LF,// 非最后一行的\n
    h_end_CR,// 最后一行的\r
    h_end_LF// 最后一行的\n
};

struct mytimer;
struct requestData;

struct requestData {
private:
    int againTimes;
    std::string path;
    int fd;
    int epollfd;
    // content的内容用完就清
    std::string content;
    int method;// 请求方法，GET 或者 POST
    int HTTPversion;// http版本号
    std::string file_name;// 访问的文件
    int now_read_pos;// 记录当前解析到的位置
    int state;// 解析报文时的状态机的状态
    int h_state;// 解析首部行的状态机的状态
    bool isfinish;
    bool keep_alive;// 报文是否要求keep_alive
    std::unordered_map<std::string, std::string> headers;// 记录首部行所有的键值对信息
    mytimer *timer;// 为该报文产生一个计时器

private:
    int parse_URI();
    int parse_Headers();
    int analysisRequest();

public:

    requestData();
    requestData(int _epollfd, int _fd, std::string _path);
    ~requestData();
    void addTimer(mytimer *mtimer);
    void reset();
    void seperateTimer();
    int getFd();
    void setFd(int _fd);
    void handleRequest();
    void handleError(int fd, int err_num, std::string short_msg);
};

struct mytimer {
    bool deleted;// 计时器是否处于关闭状态
    size_t expired_time;// 过期时间
    requestData *request_data;// 计时器绑定一个报文对象，超时时将其删除

    mytimer(requestData *_request_data, int timeout);
    ~mytimer();
    void update(int timeout);
    bool isvalid();
    void clearReq();
    void setDeleted();
    bool isDeleted() const;
    size_t getExpTime() const;
};

struct timerCmp {
    bool operator()(const mytimer *a, const mytimer *b) const;
};




#endif
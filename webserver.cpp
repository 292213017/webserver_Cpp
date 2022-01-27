#include "webserver.h"

Webserver::Webserver(){
    // 创建http_conn对象
    users = new http_conn[MAX_FD];

    // root文件的路径
    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);

    strcpy(m_root, server_path);
    strcpy(m_root,root);
    
        //定时器
    users_timer = new client_data[MAX_FD];

}

Webserver::~Webserver(){

}

void Webserver::init(int port, string user, string passWord, string databaseName, int log_write, 
                     int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model)
{
    m_port = port;
    m_user = user;
    m_passWord = passWord;
    m_databaseName = databaseName;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_log_write = log_write;
    m_OPT_LINGER = opt_linger;
    m_TRIGMode = trigmode;
    m_close_log = close_log;
    m_actormodel = actor_model;
}

void Webserver::log_write()
{
    if (0 == m_close_log)
    {
        //初始化日志
        if (1 == m_log_write)
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 800);
        else
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 0);
    }
}

// 创建数据库连接池
void Webserver::sql_pool(){
    // 创建一个数据库连接池的对象
    m_connPool = connection_pool::GetInstance();
    m_connPool->init("localhost",m_user,m_passWord,m_databaseName,3306, m_sql_num, m_close_log);

    // 初始化数据库读取数据
    users->initmysql_result(m_connPool);

}

// 创建线程池
void Webserver::thread_pool(){
    //线程池
    m_pool = new threadpool<http_conn>(m_actormodel, m_connPool, m_thread_num);
}

// 设定监听端和连接端的触发模式
void Webserver::trig_mode()
{
    //LT + LT
    if (0 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 0;
    }
    //LT + ET
    else if (1 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 1;
    }
    //ET + LT
    else if (2 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 0;
    }
    //ET + ET
    else if (3 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 1;
    }

}

void Webserver::eventListen(){
    // 创建监听文件描述符
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd > 0);

    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(m_port);

    int flags = 0;
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags));

    int ret = 0;
    ret = bind(m_listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr));
    assert(ret >= 0);

    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    // 设定超时时长
    utils.init(TIMESLOT);

    // epoll事件表创建
    epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    // 将监听事件加载到树上
    utils.addfd(m_epollfd, m_listenfd, false, m_LISTENTrigmode);
    http_conn::m_epollfd = m_epollfd;

    // 使用socketpiar创建的是一对相互连接的socket，任意一段既可以做发送，也可以做接受端
    // AF 表示ADDRESS FAMILY 地址族 
    // PF 表示PROTOCL FAMILY 协议族
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    utils.setnonblocking(m_pipefd[1]);
    utils.addfd(m_epollfd, m_pipefd[0], false, 0);

    // SIGPIPE 服务器端返回消息的时候就会收到内核给的SIGPIPE信号。 指的是客户端关闭
    // SIG_IGN ：忽略的处理方式，这个方式和默认的忽略是不一样的语意，暂且我们把忽略定义为SIG_IGN，
    // 子进程状态信息会被丢弃，也就是自动回收了，所以不会产生僵尸进程
    utils.addsig(SIGPIPE, SIG_IGN);
    // 在进行堵塞式系统调用时。为避免进程陷入无限期的等待，能够为这些堵塞式系统调用设置定时器。Linux提供了alarm系统调用和SIGALRM信号实现这个功能。
    utils.addsig(SIGALRM, utils.sig_handler, false);
    //安装终止信号
    utils.addsig(SIGTERM, utils.sig_handler, false);

    // 上面这些代码就是将信号量写到对端去

    alarm(TIMESLOT);

    //工具类,信号和描述符基础操作
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;

}

void Webserver::eventLoop()
{

}
#include "webserver.h"

Webserver::Webserver()
{
    // 创建http_conn对象
    users = new http_conn[MAX_FD];

    // root文件的路径
    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);

    strcpy(m_root, server_path);
    strcpy(m_root, root);

    //定时器
    users_timer = new client_data[MAX_FD];
}

Webserver::~Webserver()
{
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

void Webserver::timer(int connfd, struct sockaddr_in client_address)
{
    // 初始化一个http连接
    users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode, m_close_log, m_user, m_passWord, m_databaseName);

    //初始化client_data数据
    //创建定时器，设置回调函数和超时时间，绑定用户数据，将定时器添加到链表中
    users_timer[connfd].address = client_address;
    users_timer[connfd].sockfd = connfd;

    //创建定时器临时变量
    util_timer *timer = new util_timer;
    //设置定时器对应的连接资源
    timer->user_data = &users_timer[connfd];
    //设置回调函数
    timer->cb_func = cb_func;

    time_t cur = time(NULL);
    //设置绝对超时时间
    timer->expire = cur + 3 * TIMESLOT;
    //创建该连接对应的定时器，初始化为前述临时变量
    users_timer[connfd].timer = timer;
    //将该定时器添加到链表中
    utils.m_timer_lst.add_timer(timer);
}

void Webserver::deal_timer(util_timer *timer, int sockfd)
{
    //服务器端关闭连接，移除对应的定时器
    timer->cb_func(&users_timer[sockfd]);
    if (timer)
    {
        utils.m_timer_lst.del_timer(timer);
    }

    LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}

//若有数据传输，则将定时器往后延迟3个单位
//并对新的定时器在链表上的位置进行调整
void Webserver::adjust_timer(util_timer *timer)
{
    time_t cur = time(NULL);
    timer->expire = cur + 3 * TIMESLOT;
    utils.m_timer_lst.adjust_timer(timer);

    LOG_INFO("%s", "adjust timer once");
}

void Webserver::log_write()
{
    if (0 == m_close_log)
    {
        //初始化日志  1表示异步写入 0表示同步写入
        if (1 == m_log_write)
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 800);
        else
            Log::get_instance()->init("./ServerLog", m_close_log, 2000, 800000, 0);
    }
}

// 创建数据库连接池
void Webserver::sql_pool()
{
    // 创建一个数据库连接池的对象
    m_connPool = connection_pool::GetInstance();
    m_connPool->init("localhost", m_user, m_passWord, m_databaseName, 3306, m_sql_num, m_close_log);

    // 初始化数据库并初始化用户map
    users->initmysql_result(m_connPool);
}

// 创建线程池
void Webserver::thread_pool()
{
    //线程池 创建线程和分离线程
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

bool Webserver::dealclinetdata()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof(client_address);
    // 如果是LT模式
    if (0 == m_LISTENTrigmode)
    {
        // 接收文件描述符
        int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
        if (connfd < 0)
        {
            LOG_ERROR("%s:errno is:%d", "accept error", errno);
            return false;
        }
        if (http_conn::m_user_count >= MAX_FD)
        {
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        // 设置计时器
        timer(connfd, client_address);
    }
    else
    {
        while (1)
        {
            // 这里是将所有的监听一次性的拿下来
            int connfd = accept(m_listenfd, (struct sockaddr *)&client_address, &client_addrlength);
            if (connfd < 0)
            {
                LOG_ERROR("%s:errno is:%d", "accept error", errno);
                break;
            }
            if (http_conn::m_user_count >= MAX_FD)
            {
                utils.show_error(connfd, "Internal server busy");
                LOG_ERROR("%s", "Internal server busy");
                break;
            }
            timer(connfd, client_address);
        }
        return false;
    }
    return true;
}

bool Webserver::dealwithsignal(bool &timeout, bool &stop_server)
{
    int ret = 0;
    int sig;
    char signals[1024];
    //从管道读端读出信号值，成功返回字节数，失败返回-1
    //正常情况下，这里的ret返回值总是1，只有14和15两个ASCII码对应的字符
    ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if (ret == -1)
    {
        return false;
    }
    else if (ret == 0)
    {
        return false;
    }
    else
    {
        //处理信号值对应的逻辑
        for (int i = 0; i < ret; ++i)
        {
            //这里面明明是字符
            switch (signals[i])
            {
            //这里是整型
            case SIGALRM:
            {
                timeout = true;
                break;
            }
            case SIGTERM:
            {
                stop_server = true;
                break;
            }
            }
        }
    }
    return true;
}

void Webserver::dealwithread(int sockfd)
{
    //创建定时器临时变量，将该连接对应的定时器取出来
    util_timer *timer = users_timer[sockfd].timer;

    //reactor
    if (1 == m_actormodel)
    {
        if (timer)
        {
            adjust_timer(timer);
        }

        //若监测到读事件，将该事件放入请求队列
        m_pool->append(users + sockfd, 0);

        while (true)
        {
            if (1 == users[sockfd].improv)
            {
                if (1 == users[sockfd].timer_flag)
                {
                    deal_timer(timer, sockfd);
                    users[sockfd].timer_flag = 0;
                }
                users[sockfd].improv = 0;
                break;
            }
        }
    }
    else
    {
        //proactor
        if (users[sockfd].read_once())
        {
            LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));

            //若监测到读事件，将该事件放入请求队列
            m_pool->append_p(users + sockfd);

            //若有数据传输，则将定时器往后延迟3个单位
            //对其在链表上的位置进行调整
            if (timer)
            {
                adjust_timer(timer);
            }
        }
        else
        {
            //服务器端关闭连接，移除对应的定时器
            deal_timer(timer, sockfd);
        }
    }
}

// 设定监听事件
void Webserver::eventListen()
{
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
    // 绑定地址结构
    ret = bind(m_listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    assert(ret >= 0);

    // 设定监听
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
    // 设置为非阻塞
    utils.setnonblocking(m_pipefd[1]);
    // 将读事件挂到树上
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

// 服务器的运行代码
void Webserver::eventLoop()
{
    bool timeout = false;
    bool stop_server = false;

    while (!stop_server)
    {
        //监测发生事件的文件描述符
        int number = epoll_wait(m_epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR)
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        //轮询文件描述符
        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;

            //处理新到的客户连接
            if (sockfd == m_listenfd)
            {
                bool flag = dealclinetdata();
                if (false == flag)
                    continue;
            }
            //处理异常事件
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                //服务器端关闭连接，移除对应的定时器
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }
            //处理信号 //管道读端对应文件描述符发生读事件
            else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                bool flag = dealwithsignal(timeout, stop_server);
                if (false == flag)
                    LOG_ERROR("%s", "dealclientdata failure");
            }
            //处理客户连接上接收到的数据
            else if (events[i].events & EPOLLIN)
            {
                dealwithread(sockfd);
            }
            else if (events[i].events & EPOLLOUT)
            {
                dealwithwrite(sockfd);
            }
        }
        if (timeout)
        {
            utils.timer_handler();

            LOG_INFO("%s", "timer tick");

            timeout = false;
        }
    }
}
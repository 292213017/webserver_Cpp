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

}
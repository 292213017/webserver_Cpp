#include "sql_connection_pool.h"

connection_pool::connection_pool()
{
    // 存储目前的连接和空闲状态
	m_CurConn = 0;
	m_FreeConn = 0;
}

connection_pool * connection_pool::GetInstance(){
    static connection_pool connPool;
    return &connPool;
}

void connection_pool::init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, int close_log){
    m_url = url;
	m_Port = Port;
	m_User = User;
	m_PassWord = PassWord;
	m_DatabaseName = DataBaseName;
	m_close_log = close_log;

    // 创建连接数MAXconn的连接池
    for(int i = 0; i < MaxConn; i++){
        // sql数据库连接初始化
        MYSQL *con = NULL;
        con = mysql_init(con);

        if(con == NULL){
            // LOG_ERROR("Couldn't initialize connection sql'");
            exit(1);
        }

        // con连接到数据库
        con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DataBaseName.c_str(), Port, NULL, 0);

        if(con == NULL){
            // LOG_ERROR("Couldn't connection sql'");
            exit(1);
        }

        connList.push_back(con);
        ++ m_FreeConn;
    }

    // 将信号量设置为m_FreeConn，供后面--使用
    reserve = sem(m_FreeConn);
    m_MaxConn = m_FreeConn;
}


// 从数据库池中取出一个空闲的连接，更新使用和空闲的连接数
MYSQL *connection_pool::GetConnection(){
    MYSQL *con = NULL;

    if(0 == connList.size()){
        return NULL;
    }

    //等待生产者的生产
    reserve.wait();

    lock.lock();

    // 从连接池中找到list最前面的链表
    con = connList.front();
    connList.pop_front();

    -- m_FreeConn;
    ++ m_CurConn;

    lock.unlock();

    return con;

}

// 断开与数据库的连接
bool connection_pool::ReleaseConnection(MYSQL *con){
    if (NULL == con)
        return false;
    
    lock.lock();
    connList.push_back(con);
    ++m_FreeConn;
    -- m_CurConn;

    lock.unlock();

    // 相当于生产者生产产品
    reserve.post();

    return true;
}

connection_pool::~connection_pool(){

}

connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool){
	*SQL = connPool->GetConnection();
	
	conRAII = *SQL;
	poolRAII = connPool;
}

connectionRAII::~connectionRAII(){
	poolRAII->ReleaseConnection(conRAII);
}
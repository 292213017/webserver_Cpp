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

        con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DataBaseName.c_str(), Port, NULL, 0);

        if(con == NULL){
            // LOG_ERROR("Couldn't connection sql'");
            exit(1);
        }

        connList.push_back(con);
        ++ m_FreeConn;
    }

    m_MaxConn = m_FreeConn;
}
// reserve = sem(m_FreeConn);

connection_pool::~connection_pool(){

}
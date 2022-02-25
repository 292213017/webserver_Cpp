#include "http_conn.h"

map<string, string> users;

void http_conn::initmysql_result(connection_pool *connPool){
    // 首先从连接池中取出一个连接
    MYSQL *mysql = NULL;

    // RAll机制，从sql连接池中，取出一个sql连接
    connectionRAII mysqlcon(&mysql, connPool);

    // mysql_query 这个可以调用sql语句
    if (mysql_query(mysql, "SELECT username,passwd FROM user"))
    {
        printf("SELECT error:%s\n", mysql_error(mysql));
        exit(1);
    }

    // 从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    // 返回结果集中的列数
    int num_fields = mysql_num_fields(result);

    //返回所有字段结构的数组
    MYSQL_FIELD *fields = mysql_fetch_fields(result);

    //从结果集中获取下一行，将对应的用户名和密码，存入map中
    while (MYSQL_ROW row = mysql_fetch_row(result))
    {
        string temp1(row[0]);
        string temp2(row[1]);
        users[temp1] = temp2;
    }
}

int http_conn::m_epollfd = -1;
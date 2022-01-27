#include "../CGImysql/sql_connection_pool.h"
#include <sys/socket.h>
#include <sys/types.h>          /* See NOTES */


class http_conn{
    public:
        static const int FILENAME_LEN = 200;
        static const int READ_BUFFER_SIZE = 2048;
        static const int WRITE_BUFFER_SIZE = 1024;

    public:
        http_conn(){};
        ~http_conn(){};

        void initmysql_result(connection_pool *connPool);
    public:
    static int m_epollfd;
    static int m_user_count;
    MYSQL *mysql;
    int m_state;  //读为0, 写为1

    private:
        int m_sockfd;
        // struct sockaddr_in m_address;
        char m_read_buf[READ_BUFFER_SIZE];
        int m_read_idx;
        int m_checked_idx;
        int m_start_line;
        char m_write_buf[WRITE_BUFFER_SIZE];
        int m_write_idx;
        // CHECK_STATE m_check_state;
        // METHOD m_method;
        char m_real_file[FILENAME_LEN];
        char *m_url;
        char *m_version;
        char *m_host;
        int m_content_length;
        bool m_linger;
        char *m_file_address;
        // struct stat m_file_stat;
        // struct iovec m_iv[2];
        int m_iv_count;
        int cgi;        //是否启用的POST
        char *m_string; //存储请求头数据
        int bytes_to_send;
        int bytes_have_send;
        char *doc_root;

        map<string, string> m_users;
        int m_TRIGMode;
        int m_close_log;

        char sql_user[100];
        char sql_passwd[100];
        char sql_name[100];
};
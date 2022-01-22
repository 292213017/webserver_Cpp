#include "../CGImysql/sql_connection_pool.h"


class http_conn{
    public:
        http_conn(){};
        ~http_conn(){};

        void initmysql_result(connection_pool *connPool);
};
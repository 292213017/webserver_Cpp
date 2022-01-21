#include "config.h"

using namespace std;

int main(int argc, char **argv){

    string user = "root";
    string passwd ="root";
    string databasename = "testdb";

    Config config;
    config.parse_args(argc, argv);

    Webserver webserver;

    // 服务器初始化
    webserver.init(config.PORT, user, passwd, databasename, config.LOGWrite, 
                config.OPT_LINGER, config.TRIGMode,  config.sql_num,  config.thread_num, 
                config.close_log, config.actor_model);

    // 日志文件初始化
    webserver.log_write();

    //数据库
    webserver.sql_pool();
    

    return 0;
}
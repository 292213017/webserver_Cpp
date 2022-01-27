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
    
    //线程池
    webserver.thread_pool();

    //触发模式
    webserver.trig_mode();

    // 监听启动
    webserver.eventListen();

    //运行
    webserver.eventLoop();

    return 0;
}
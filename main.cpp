#include "config.h"

using namespace std;

int main(int argc, char **argv){

    string user = "root";
    string passwd ="root";
    string databasename = "testdb";

    Config config;
    config.parse_args(argc, argv);

    Webserver webserver;

    webserver.init(config.PORT, user, passwd, databasename, config.LOGWrite, 
                config.OPT_LINGER, config.TRIGMode,  config.sql_num,  config.thread_num, 
                config.close_log, config.actor_model);


    webserver.log_write();
    

    return 0;
}
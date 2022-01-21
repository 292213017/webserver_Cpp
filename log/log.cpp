#include "log.h"

Log::Log()
{
    m_count = 0;
    m_is_async = false;
}

Log::~Log()
{
    if (m_fp != NULL)
    {
        fclose(m_fp);
    }
}

bool Log::init(const char *file_name, int close_log, int log_buf_size, int split_lines, 
                            int max_queue_size){

    if(max_queue_size > -1){

    }

    m_close_log = close_log;
    m_log_buf_size = log_buf_size;
    m_buf = new char[m_log_buf_size];
    memset(m_buf, '\0', m_log_buf_size);
    m_split_lines = split_lines;

    time_t t = time(NULL);
    struct tm *sys_tm = localtime(&t);
    struct tm my_tm = *sys_tm;

    //C 库函数 char *strrchr(const char *str, int c) 在参数 str 所指向的字符串中搜索最后一次出现字符 c（一个无符号字符）的位置。
    //根据实践拼一个log文件名
    const char *p = strrchr(file_name, '/');
    char log_full_name[256] = {0};

    strcpy(log_name, p + 1);
    strncpy(dir_name, file_name, p - file_name +1);
    snprintf(log_full_name,300, "%s%d_%02d_%02d_%s", dir_name, my_tm.tm_year + 1900, my_tm.tm_mon + 1, my_tm.tm_mday, log_name);

    m_today = my_tm.tm_mday;

    m_fp = fopen(log_full_name, "a");
    if (m_fp == NULL)
    {
        perror("create log file error");
        return false;
    }

    return true;

}

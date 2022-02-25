#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/lock.h"
#include "../CGImysql/sql_connection_pool.h"

template <typename T>
class threadpool{
public:
    threadpool(int actor_model, connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    // 这里之所以用静态，因为它要作为回调函数使用，但是他又在一个类内，所以设置为static形式，消除this指针的问题
    static void *worker(void *arg);
    void run();

private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t *m_threads;       //描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue; //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuestat;            //是否有任务需要处理
    connection_pool *m_connPool;  //数据库
    int m_actor_model;          //模型切换
};

template<typename T>
threadpool<T>::threadpool(int actor_model, connection_pool *connPool, int thread_number, int max_request)
: m_actor_model(actor_model),m_thread_number(thread_number), m_max_requests(max_request), m_threads(NULL),m_connPool(connPool)
{
        // 如果现在的进程数和最大请求数不是合法值
        if (thread_number <= 0 || max_request <= 0)
            throw std::exception();

        // 创建一个线程数组
        m_threads = new pthread_t[thread_number];
        if (!m_threads)
            throw std::exception();
        
        for(int i = 0; i < thread_number; ++ i){
            // 创建线程
            if(pthread_create(m_threads+ 1, NULL, worker, this) != 0){
                delete[] m_threads;
                throw std::exception();
            }
            //分离线程
            if(pthread_detach(m_threads[i])){
                delete[] m_threads;
                throw std::exception();
            }
        }
}

template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
}

// 线程的回调函数
template <typename T>
void *threadpool<T>::worker(void *arg){
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}

// 取出线程，开始工作
template <typename T>
void threadpool<T>::run()
{
    // while(true) {
    //     m_queuestat.wait();
    //     m_queuestat.lock();

    //     m_queuestat.unlock();
    // }
}


#endif
#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../sql/sql_connection_pool.h"

template<typename T>
class threadpool{
public:    
    /**
     * @brief Construct a new threadpool object
     * 
     * @param actor_model 
     * @param connPool 
     * @param thread_number 线程池中线程的数量
     * @param max_request 请求队列中最多允许的、等待处理的请求的数量
     */
    threadpool(int actor_model, connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T *request, int state);
    bool append_p(T *request);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行*/
    static void *worker(void *arg); // why static???    class specific
    void run();

private:
    int m_thread_number;         // 线程池中的线程数 
    int m_max_requests;          // 请求队列中允许的最大请求数
    pthread_t *m_threads;        // 描述线程池的数组，其大小为m_thread_number
    std::list<T *> m_workqueue;  // 请求队列
    locker m_queuelocker;        // 保护请求队列的互斥锁
    sem m_queuestat;             // 是否有任务需要处理
    connection_pool *m_connPool; // 数据库
    int m_actor_model;           // 模型切换 这个切换是指的Reactor/Proactor
};

// 线程池构造函数
template<typename T>
threadpool<T>::threadpool(int actor_model, connection_pool *connPool, int thread_number, int max_request)
    : m_actor_model(actor_model), m_thread_number(thread_number), m_max_requests(max_requests), m_threads(NULL), m_connPool(connPool){

        if (thread_number <= 0 || max_requests <= 0)
            throw std::exception();
        
        m_threads = new pthread_t[m_thread_number];  // pthreat_t 是长整型

        if (!m_threads)
            throw std::exception();

        for (int i = 0; i < thread_number; ++ i){
            // 函数原型中的第三个参数，为函数指针，指向处理线程函数的地址
            // 若线程函数为类成员函数 则this指针会作为默认的参数被传进函数中，从而和线程函数参数(void *)不能匹配，不能通过编译
            // 静态成员函数就没有这个问题，因为里面没有this指针
            if (pthread_create(m_threads + i, NULL, worker, this) != 0){

                delete[] m_threads;
                throw std::exception();
            }

            // 主要是将线程属性更改为unjoinable，使得主线程分离，便于资源的释放
            if (pthread_detach(m_threads[i])){

                delete[] m_threads;
                throw std::exception();
            }
        }
    }

// 析构函数
template<typename T>
threadpool<T>::~threadpool(){

    delete[] m_threads;
}

// Reactor模式下的请求入队
template<typename T>
bool threadpool<T>::append(T *request, int state){

    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests){

        m_queuelocker.unlock();
        return false;
    }

    // 读写事件
    request->m_state = state; // ??做什么的
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

// Proactor模式下的请求入队
template<typename T>
bool threadpool<T>::append_p(T *request){

    m_queuelocker.lock();
    if (m_workqueue.size() >= m_max_requests){

        m_queuelocker.unlock();
        return false;
    }

    // 对比reactor  少了state
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

// 工作线程  创建线程的时候需要调用
template <typename T>
void *threadpool<T>::worker(void *arg){

    // 调用的时候 *arg是this！
    // 所以该操作其实是获取threadpool对象地址
    threadpool *pool = (threadpool *)arg;
    // 线程池中每一个线程创建时都会调用run(),睡眠在队列中
    // 怎么睡眠的？？？
    pool->run();
    return pool;
}

// 线程池中的所有线程都睡眠，等待请求队列中新增任务
template <typename T>
void threadpool<T>::run(){

    while (true) {

        m_queuestat.wait();
        m_queuelocker.lock();
        if (m_workqueue.empty()){

            m_queuelocker.unlock();
            continue;
        }

        // 
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request)
            continue;
        // Reactor
        if (1 == m_actor_model){

            if (0 == request->m_state){

                if (request->read_once()){

                    request->improv = 1;
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
                    request->process();
                } else {

                    request->improv = 1;
                    request->timer_flag = 1;
                }
            } else {

                if (request->write()){

                    request->improv = 1;
                } else {

                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        } else { // default:Proactor

            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }
    }
}

#endif
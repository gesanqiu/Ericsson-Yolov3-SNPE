#ifndef __TS_PROANDCON_H__
#define __TS_PROANDCON_H__

#include <list>
#include <mutex>
#include <condition_variable>
#include <iostream>

using namespace std;

template<typename T>
class TsProAndCon {
private:
    list<std::shared_ptr<T>> m_queue;
    mutex m_mutex;//全局互斥锁
    condition_variable_any m_notEmpty;//全局条件变量（不为空）
    condition_variable_any m_notFull;//全局条件变量（不为满）
    unsigned int m_maxSize;//队列最大容量

private:
    //队列为空
    bool isEmpty() const {
        return m_queue.empty();
    }
    //队列已满
    bool isFull() const {
        return m_queue.size() == m_maxSize;
    }

public:
    TsProAndCon(unsigned int maxSize) {
        this->m_maxSize = maxSize;
    }
    TsProAndCon(){
        this->m_maxSize = 25;
    }

    int queuecount() const {
        return m_queue.size();
    }

    void setMaxSize(unsigned int maxSize) {
        this->m_maxSize = maxSize;
    }

    unsigned int getMaxSize() {
        return this->m_maxSize;
    }

    unsigned int getCurrentSize() {
        return m_queue.size();
    }

    ~TsProAndCon(){}

    void product(const std::shared_ptr<T>& v) {
        unique_lock<mutex> locker(m_mutex);
        while(isFull()) {
            //cout<< debug_info << "队列已满，请等待"<<endl;
            //生产者等待"产品队列缓冲区不为满"这一条件发生.
            m_notFull.wait(m_mutex);
        }
        //往队列里面生产一个元素,同时通知不为空这个信号量
        m_queue.push_back(v);
        m_notEmpty.notify_one();
    }
    void consumption(std::shared_ptr<T>& v) {
        unique_lock<mutex> locker(m_mutex);
        while(isEmpty()) {
            //cout<< debug_info << "队列已空，请等待"<<endl;
            // 消费者等待"产品队列缓冲区不为空"这一条件发生.
            m_notEmpty.wait(m_mutex);
        }
        //在队列里面消费一个元素,同时通知队列不满这个信号量
        v = m_queue.front();
        m_queue.pop_front();
        m_notFull.notify_one();
    }

    std::string debug_info;
};


#endif

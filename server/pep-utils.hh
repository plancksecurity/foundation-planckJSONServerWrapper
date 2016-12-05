#pragma once

#include <list>
#include <pthread.h>
#include <errno.h>

namespace pEp {
    namespace utility {
        using namespace std;

        class mutex {
            typedef pthread_mutex_t native_handle_type;
            native_handle_type _mutex;

        public:
            mutex() {
                int result;
                do {
                    result = pthread_mutex_init(&_mutex, NULL);
                } while (result == EAGAIN);
            }
            ~mutex() {
                pthread_mutex_destroy(&_mutex);
            }
            void lock() {
                pthread_mutex_lock(&_mutex);
            }
            void unlock() {
                pthread_mutex_unlock(&_mutex);
            }
            native_handle_type native_handle() {
                return _mutex;
            }
            bool try_lock() {
                return pthread_mutex_trylock(&_mutex) == 0;
            }
        };

        template<class T> class lock_guard {
            T& _mtx;

        public:
            lock_guard(T& mtx) : _mtx(mtx) {
                _mtx.lock();
            }
            ~lock_guard() {
                _mtx.unlock();
            }
        };

        template<class T> class locked_queue
        {
            mutex _mtx;
            list<T> _q;

        public:
            T& back()
            {
                lock_guard<mutex> lg(_mtx);
                return _q.back();
            }
            T& front()
            {
                lock_guard<mutex> lg(_mtx);
                return _q.front();
            }
            void clear()
            {
                lock_guard<mutex> lg(_mtx);
                _q.clear();
            }
            void pop_back()
            {
                lock_guard<mutex> lg(_mtx);
                _q.pop_back();
            }
            void pop_front()
            {
                lock_guard<mutex> lg(_mtx);
                _q.pop_front();
            }
            void push_back(const T& data)
            {
                lock_guard<mutex> lg(_mtx);
                _q.push_back(data);
            }
            void push_front(const T& data)
            {
                lock_guard<mutex> lg(_mtx);
                _q.push_front(data);
            }
            size_t size()
            {
                lock_guard<mutex> lg(_mtx);
                return _q.size();
            }
            bool empty()
            {
                lock_guard<mutex> lg(_mtx);
                return _q.empty();
            }
        };
    }
};


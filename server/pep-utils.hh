#pragma once

#include <list>
#include <mutex>
#include <cerrno>

namespace pEp {
    namespace utility {

        template<class T> class locked_queue
        {
            typedef std::recursive_mutex Mutex;
            typedef std::lock_guard<Mutex> Lock;
            
            Mutex _mtx;
            std::list<T> _q;

        public:
            T& back()
            {
                Lock L(_mtx);
                return _q.back();
            }
            T& front()
            {
                Lock L(_mtx);
                return _q.front();
            }
            void clear()
            {
                Lock L(_mtx);
                _q.clear();
            }
            void pop_back()
            {
                Lock L(_mtx);
                _q.pop_back();
            }
            void pop_front()
            {
                Lock L(_mtx);
                _q.pop_front();
            }
            void push_back(const T& data)
            {
                Lock L(_mtx);
                _q.push_back(data);
            }
            void push_front(const T& data)
            {
                Lock L(_mtx);
                _q.push_front(data);
            }
            size_t size()
            {
                Lock L(_mtx);
                return _q.size();
            }
            bool empty()
            {
                Lock L(_mtx);
                return _q.empty();
            }
        };
    }
}


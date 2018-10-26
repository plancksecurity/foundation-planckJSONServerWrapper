#pragma once

#include <list>
#include <mutex>
#include <condition_variable>
#include <string>

namespace pEp
{
	namespace utility
	{
		template<class T>
		std::unique_ptr<T, void(*)(T*)> make_c_ptr(T* data, void(*Deleter)(T*))
		{
			return { data, Deleter };
		}
	
#if 0
		// a thread-safe queue of T elements. Deleter is a functor that is called in clear() for each elements
		// interface differs from std::queue because "top() and pop() if not empty()" does not work atomically!
		// elements must be copied without exceptions!
		// pop() blocks when queue is empty
		template<class T, void(*Deleter)(T)> class locked_queue
		{
			typedef std::recursive_mutex Mutex;
			typedef std::unique_lock<Mutex> Lock;
			
			Mutex _mtx;
			std::condition_variable_any _cv;
			std::list<T> _q;

		public:
			~locked_queue()
			{
				clear();
			}
			
			void clear()
			{
				Lock L(_mtx);
				for(auto element : _q)
				{
					Deleter(element);
				}
				_q.clear();
			}
			
			// returns a copy of the last element.
			// blocks when queue is empty
			T pop_back()
			{
				Lock L(_mtx);
				_cv.wait(L, [&]{ return !_q.empty(); } );
				T ret{std::move(_q.back())};
				_q.pop_back();
				return ret;
			}
			
			// returns a copy of the first element.
			// blocks when queue is empty
			T pop_front()
			{
				Lock L(_mtx);
				_cv.wait(L, [&]{ return !_q.empty(); } );
				T ret{std::move(_q.front())};
				_q.pop_front();
				return ret;
			}
			
			// returns true and set a copy of the last element and pop it from queue if there is any
			// returns false and leaves 'out' untouched if queue is empty even after 'timeout' milliseconds
			bool try_pop_back(T& out, std::chrono::steady_clock::time_point end_time)
			{
				Lock L(_mtx);
				if(! _cv.wait_until(L, end_time, [this]{ return !_q.empty(); } ) )
				{
					return false;
				}
				
				out = std::move(_q.back());
				_q.pop_back();
				return true;
			}
			
			// returns true and set a copy of the first element and pop it from queue if there is any
			// returns false and leaves 'out' untouched if queue is empty even after 'timeout' milliseconds
			bool try_pop_front(T& out, std::chrono::steady_clock::time_point end_time)
			{
				Lock L(_mtx);
				if(! _cv.wait_until(L, end_time, [this]{ return !_q.empty(); } ) )
				{
					return false;
				}
				
				out = std::move(_q.front());
				_q.pop_front();
				return true;
			}
			
			void push_back(const T& data)
			{
				{
					Lock L(_mtx);
					_q.push_back(data);
				}
				_cv.notify_one();
			}
			
			void push_front(const T& data)
			{
				{
					Lock L(_mtx);
					_q.push_front(data);
				}
				_cv.notify_one();
			}
			
			void push_back(T&& data)
			{
				{
					Lock L(_mtx);
					_q.push_back( std::move(data) );
				}
				_cv.notify_one();
			}
			
			void push_front(T&& data)
			{
				{
					Lock L(_mtx);
					_q.push_front( std::move(data) );
				}
				_cv.notify_one();
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
#endif

	} // end of namespace pEp::util
} // end of namespace pEp


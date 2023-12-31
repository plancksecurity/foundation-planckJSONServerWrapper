#include <stdexcept>
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <cstdint>
#include <vector>
#include <pEp/webserver.hh>

#include <string>
#include <set>
#include <fcntl.h>
#include <functional>
#include <tuple>

#include "json-adapter.hh"
#include "daemonize.hh"
#include "pEp-types.hh"
#include "json_rpc.hh"
#include "security-token.hh"
#include "pEp-utils.hh"
#include "ev_server.hh"
#include "logger.hh"
#include "server_version.hh"
#include "session_registry.hh"

#include <pEp/keymanagement.h>
#include <pEp/call_with_lock.hh>
#include <pEp/constant_time_algo.hh>
#include <pEp/status_to_string.hh>  // from libpEpAdapter.
#include <pEp/locked_queue.hh>

#include <boost/filesystem.hpp>
#include "json_spirit/json_spirit_writer.h"
#include "json_spirit/json_spirit_reader.h"
#include "json_spirit/json_spirit_utils.h"

// 19.08.2023/DZ - Pass the notifyHandshake listener on to the session registry.

#if (__cplusplus >= 201606)  // std::variant is C++17.
#   include <variant>
    using std::variant;
    using std::get;

#else // in C++11 / C++14 use boost::variant instead.
#   include <boost/variant.hpp>
    using boost::variant;
    using boost::get;
#endif


namespace fs = boost::filesystem;


namespace {
    using namespace pEp::utility;


const std::string BaseUrl    = "/ja/0.1/";

const uint64_t Guard_0 = 123456789;
const uint64_t Guard_1 = 987654321;


typedef std::recursive_mutex     Mutex;
typedef std::unique_lock<Mutex>  Lock;
Mutex  _mtx;


typedef variant<std::thread::id, std::string> EventListenerKey;

struct EventListenerValue
{
	std::shared_ptr<utility::locked_queue<js::Object>> Q = std::make_shared<utility::locked_queue<js::Object>>();;
};

static std::hash<std::thread::id> hash_tid;

const js::Object queue_close_event{ {"queue_close",true} };

} // end of anonymous namespace


// *sigh* necessary because messageToSend() has no obj pointer anymore. :-(
JsonAdapter* JsonAdapter::singleton = nullptr;


struct JsonAdapter::Internal
{
	std::unique_ptr<SessionRegistry> session_registry{};
	std::string token;
	std::map<EventListenerKey, EventListenerValue> eventListener;
	
	Logger      Log;
	std::unique_ptr<pEp::Webserver> webserver;
	inject_sync_event_t inject_sync_event = nullptr;
	
	unsigned    request_count = 0;
	bool        running = false;
	bool        silent  = false;
	bool        ignore_session_error = false;
	bool        deliver_html = true;
	
	int  client_session_timeout = 7*60; // in seconds
	
	explicit Internal()
	: Log("JAI")
	{}
	
	Internal(const Internal&) = delete;
	void operator=(const Internal&) = delete;
	
	~Internal()
	{
		DEBUG_LOG(Log) << "~JAI";
	}
	
	
	std::string to_log(const EventListenerKey& v)
	{
		const std::thread::id* tid = get<const std::thread::id>(&v);
		if(tid)
		{
			return Logger::thread_id(hash_tid(*tid));
		}else{
			const std::string* s = get<const std::string>(&v);
			if(s)
			{
				return "<<" + *s + ">>";
			}
		}
		
		return "(?)";
	}
	
	void makeAndDeliverRequest(const char* function_name, const js::Array& params)
	{
		const js::Object request = make_request( function_name, params);
		const std::string request_r = js::write(request);
		
		Lock L(_mtx);
		DEBUG_LOG(Log) << "makeAndDeliverRequest: \n"
			"Request: " << request_r << "\n"
			"Goes to " << eventListener.size() << " listener(s).";
		
		for(auto& e : eventListener)
		{
			DEBUG_LOG(Log) << " ~~~ " << to_log(e.first)  << " has " << e.second.Q->size() << " old events waiting.";
			e.second.Q->push_back(request);
		}
	}
	
	js::Array pollForEvents(const EventListenerKey& key, unsigned timeout_seconds);

};


PEP_SESSION JsonAdapter::getSessionForThread(const std::string& client_id)
{
	const auto thread_id = std::this_thread::get_id();
	return JsonAdapter::getInstance().i->session_registry->get(thread_id, client_id);
}


In_Pep_Session::In_Pep_Session(const js::Value& v, Context* ctx, unsigned)
: Base( JsonAdapter::getSessionForThread(ctx->client_id()) )
{}


template<>
JsonAdapter* from_json(const js::Value& /* not used */)
{
	return &JsonAdapter::getInstance();
}


In<JsonAdapter*, ParamFlag::NoInput>::In(const js::Value&, Context*, unsigned)
: Base{ &JsonAdapter::getInstance() }
{
	// nothing to do here. :-D
}

template<>
js::Value Type2String<In_Pep_Session>::get() { throw "Make MSVC happy again. m("; }


ServerVersion JsonAdapter::version()
{
	return server_version();
}



PEP_STATUS JsonAdapter::messageToSend(message* msg)
{
	JsonAdapter& ja = getInstance();
	js::Value v{to_json(msg)};
	ja.i->makeAndDeliverRequest("messageToSend", js::Array{ std::move(v) } );
	return PEP_STATUS_OK;
}


PEP_STATUS JsonAdapter::notifyHandshake(pEp_identity* self, pEp_identity* partner, sync_handshake_signal sig)
{
	js::Array param_array;
	param_array.emplace_back( to_json(self) );
	param_array.emplace_back( to_json(partner) );
	param_array.emplace_back( to_json(sig) );
	getInstance().i->makeAndDeliverRequest("notifyHandshake", param_array );
	return PEP_STATUS_OK;
}


JsonAdapter::JsonAdapter()
: guard_0(Guard_0)
, i(new Internal{})
, guard_1(Guard_1)
{
	// nothing to do here.
}


JsonAdapter::~JsonAdapter()
{
	check_guard();
	Log() << "~JsonAdapter(): " << i->session_registry->size() << " sessions registered.";
	this->shutdown(nullptr);
	Log() << "\t After stopSync() and shutdown() there are " << i->session_registry->size() << " sessions registered.";
	delete i;
	i=nullptr;
}


JsonAdapter& JsonAdapter::ignore_session_errors(bool _ig)
{
	check_guard();
	i->ignore_session_error = _ig;
	return *this;
}


JsonAdapter& JsonAdapter::deliver_html(bool dh)
{
	check_guard();
	i->deliver_html = dh;
	return *this;
}


JsonAdapter& JsonAdapter::set_client_session_timeout(int timeout_seconds)
{
	check_guard();
	i->client_session_timeout = timeout_seconds;
	return *this;
}


void JsonAdapter::prepare_run(const std::string& address, unsigned start_port, unsigned end_port, ::messageToSend_t messageToSend)
{
	check_guard();
	// delayed after constructor, so virtual functions are working:
	i->session_registry.reset(new SessionRegistry(messageToSend ? messageToSend : this->getMessageToSend(), notifyHandshake, this->getInjectSyncEvent(), i->client_session_timeout));
	
	for(unsigned short port = start_port; port<=end_port; ++port)
	{
		try{
			i->webserver = std::make_unique<ev_server>(address, port, i->deliver_html, BaseUrl);
			break;
		}catch(...)
		{
			// okay, next port!
		}
	}
	
	if(!i->webserver)
	{
		throw std::runtime_error("Cannot bind to a port between " + std::to_string(start_port)
			+ " and " + std::to_string(end_port) + "." );
	}
	
	i->token = create_security_token(address, i->webserver->port(), BaseUrl);
	
	Log() << "Bound to port " << i->webserver->port() << ", sec_token=\"" << i->token << "\".";
}


void JsonAdapter::run()
try
{
	check_guard();
	Logger L("JA:run");
	
	L << Logger::Info << "This is " << (void*)this << ", thread id " << std::this_thread::get_id() << ".";
	DEBUG_LOG(L) << "Registry:\n" << i->session_registry->to_string();
	
	i->running = true;
	i->webserver->run();
}
catch (std::exception const &e)
{
	Log(Logger::Error) << "Exception in JsonAdapter::run(): \"" << e.what() << "\"";
	throw;
}


void JsonAdapter::connection_close_cb()
{
	Lock L{_mtx};
	const auto tid = std::this_thread::get_id();
	auto q = i->eventListener.find( tid );
	DEBUG_LOG(i->Log) << "Connection Close Callback: " << (q==i->eventListener.end() ? "NO" : "1") << " entry in eventListener map for Thread " << tid << ".";
	if(q != i->eventListener.end())
	{
		i->Log.debug("%d listener(s) waiting on event queue", q->second.Q->waiting());
		while(q->second.Q->waiting() > 0)
		{
			q->second.Q->push_back(queue_close_event);
			std::this_thread::sleep_for( std::chrono::milliseconds(333) );
		}
		i->eventListener.erase(q);
	}
}


void JsonAdapter::close_session(const std::string& session_id)
{
	Lock L{_mtx};
	auto q = i->eventListener.find( session_id );
	DEBUG_LOG(i->Log) << "Close session \"" << session_id << "\": " << (q==i->eventListener.end() ? "NO" : "1") << " entry in eventListener map for session_id \"" << session_id << "\".";
	if(q != i->eventListener.end())
	{
		DEBUG_OUT(i->Log, "%d listener(s) waiting on event queue", q->second.Q->waiting());
		while(q->second.Q->waiting() > 0)
		{
			q->second.Q->push_back(queue_close_event);
			std::this_thread::sleep_for( std::chrono::milliseconds(333) );
		}
		i->eventListener.erase(q);
	}
}


std::string JsonAdapter::create_session()
{
	std::string rt = create_random_token(12);
	return rt;
}


void JsonAdapter::shutdown(timeval* t)
{
	check_guard();
	Log() << "JS::shutdown() was called.";
	i->running = false;
	i->webserver->shutdown();
}


// returns 'true' if 's' is the security token created by the function above.
bool JsonAdapter::verify_security_token(const std::string& s) const
{
	check_guard();
	const bool eq = pEp::constant_time_equal(s, i->token);
	if( eq==false )
	{
		Log(Logger::Notice) << "sec_token=\"" << i->token << "\" (len=" << i->token.size() << ") is unequal to \"" << s << "\" (len=" << s.size() << ")!";
	}
	return eq;
}


void JsonAdapter::cache(const std::string& client_id, const std::string& fn_name, const std::function<void(PEP_SESSION)>& func)
{
	i->session_registry->add_to_cache(client_id, fn_name, func);
}


js::Array JsonAdapter::pollForEvents(unsigned timeout_seconds)
{
	return i->pollForEvents( std::this_thread::get_id(), timeout_seconds);
}


js::Array JsonAdapter::pollForEvents2(const std::string& session_id, unsigned timeout_seconds)
{
	return i->pollForEvents( session_id, timeout_seconds);
}


js::Array JsonAdapter::Internal::pollForEvents(const EventListenerKey& key, unsigned timeout_seconds)
{
	js::Array arr{};
	Logger L("JAI:poll");

	Lock LCK{_mtx};
	auto elQ = eventListener[key].Q;  // adds an entry, if not already there. :-)
	LCK.unlock();
	
	const size_t size = elQ->size();
	if(size)
	{
		DEBUG_LOG(L) << size << " events in queue for key " << to_log(key) << ":";
		// fetch all elements from queue
		for(size_t i=0; i<size; ++i)
		{
			js::Object obj{ elQ->pop_front() };
			const std::string obj_s = js::write( obj );
			DEBUG_LOG(L) << "\t#" << i << ": " << obj_s;
			
			arr.emplace_back( std::move(obj) );
		}
	}else{
		// block until there is at least one element or timeout
		DEBUG_LOG(L) << "Queue for key " << to_log(key) << " is empty. I'll block for " << timeout_seconds << " seconds.";
		js::Object event;
		const bool success = elQ->try_pop_front( event, std::chrono::seconds(timeout_seconds) );
		if(success)
		{
			const std::string event_s = js::write(event);
			DEBUG_LOG(L) << "Success! Got this event: " << event_s ;
			arr.emplace_back( std::move(event) );
		}else{
			DEBUG_LOG(L) << "Timeout. No event after " << timeout_seconds << " seconds arrived. So sad.";
		}
	}
	
	DEBUG_LOG(L) << "Return array with " << arr.size() << " elements.";
	return arr;
}


Logger::Stream JsonAdapter::Log(Logger::Severity sev) const
{
	check_guard();
	return std::move(i->Log << sev);
}


bool JsonAdapter::running() const
{
	return i->running;
}


void JsonAdapter::check_guard() const
{
	if(guard_0 != Guard_0 || guard_1 != Guard_1 || i==nullptr)
	{
		char buf[128];
		snprintf(buf,127, "JS::check_guard failed: guard0=%llu, guard1=%llu this=%p i=%p.\n",
			guard_0, guard_1, (void*)this, (void*)i
			);
		std::cerr << buf; // Log() might not work here, when memory is corrupted
		throw std::logic_error( buf );
	}
}


std::recursive_mutex get_instance_mutex;

JsonAdapter& JsonAdapter::createInstance(JsonAdapter* instance)
{
	std::lock_guard<std::recursive_mutex> L(get_instance_mutex);
	
	if(!singleton)
	{
		singleton = instance;
	}
	
	return *singleton;
}


JsonAdapter& JsonAdapter::getInstance()
{
	if(!singleton)
	{
		throw std::logic_error("You forgot to call JsonAdapter::createInstance() before!");
	}
	
	return *singleton;
}


SessionRegistry& JsonAdapter::getSessionRegistry()
{
	return *(singleton->i->session_registry.get());
}


JsonAdapter& JsonAdapter::startup(::messageToSend_t messageToSend = nullptr)
{
	JsonAdapter& ja = getInstance();
	ja.prepare_run("127.0.0.1", 4223, 9999, messageToSend);
	ja.run();
	return ja;
}

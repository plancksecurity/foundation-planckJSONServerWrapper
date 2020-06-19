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

#include <pEp/keymanagement.h>
#include <pEp/Adapter.hh>
#include <pEp/status_to_string.hh>  // from libpEpAdapter.
#include <pEp/locked_queue.hh>

#include <boost/filesystem.hpp>
#include "json_spirit/json_spirit_writer.h"
#include "json_spirit/json_spirit_reader.h"
#include "json_spirit/json_spirit_utils.h"


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
	utility::locked_queue<js::Object> Q;
};

static std::hash<std::thread::id> hash_tid;

} // end of anonymous namespace


// *sigh* necessary because messageToSend() has no obj pointer anymore. :-(
JsonAdapter* JsonAdapter::singleton = nullptr;


struct JsonAdapter::Internal
{
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
	
	explicit Internal()
	: Log("JAI")
	{}
	
	Internal(const Internal&) = delete;
	void operator=(const Internal&) = delete;
	
	~Internal()
	{
		Log << Logger::Debug << "~JAI";
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
		Log << Logger::Debug << "makeAndDeliverRequest: \n"
			"Request: " << request_r << "\n"
			"Goes to " << eventListener.size() << " listener(s).";
		
		for(auto& e : eventListener)
		{
			Log << Logger::Debug << " ~~~ " << to_log(e.first)  << " has " << e.second.Q.size() << " old events waiting.";
			e.second.Q.push_back(request);
		}
	}
	
	js::Array pollForEvents(const EventListenerKey& key, unsigned timeout_seconds);

};


PEP_SESSION JsonAdapter::getSessionForThread()
{
	return pEp::Adapter::session(Adapter::init);
}


In_Pep_Session::In_Pep_Session(const js::Value& v, Context*, unsigned)
: Base( JsonAdapter::getSessionForThread() )
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
	js::Value v{to_json(msg)};
	getInstance().i->makeAndDeliverRequest("messageToSend", js::Array{ std::move(v) } );
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
	Log() << "~JsonAdapter().";
	this->shutdown(nullptr);
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


void JsonAdapter::prepare_run(const std::string& address, unsigned start_port, unsigned end_port)
{
	check_guard();
	// delayed after constructor, so virtual functions are working:
	pEp::Adapter::startup(this->getMessageToSend(), &JsonAdapter::notifyHandshake);
	
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
	auto q = i->eventListener.find( std::this_thread::get_id() );
	Log() << "Connection Close Callback: " << (q==i->eventListener.end() ? "NO" : "1") << " entry in eventListener map";
	if(q != i->eventListener.end())
	{
		i->eventListener.erase(q);
	}
}


void JsonAdapter::close_session(const std::string& session_id)
{
	Lock L{_mtx};
	auto q = i->eventListener.find( session_id );
	Log() << "Close session \"" << session_id << "\": " << (q==i->eventListener.end() ? "NO" : "1") << " entry in eventListener map";
	if(q != i->eventListener.end())
	{
		i->eventListener.erase(q);
	}
}


std::string JsonAdapter::create_session()
{
	return create_random_token(12);
}


void JsonAdapter::shutdown(timeval* t)
{
	exit(0);  // HACK for JSON-41
	check_guard();
	Log() << "JS::shutdown() was called.";
	i->running = false;
	i->webserver->shutdown();
}


// returns 'true' if 's' is the security token created by the function above.
bool JsonAdapter::verify_security_token(const std::string& s) const
{
	check_guard();
	if(s!=i->token)
	{
		Log(Logger::Notice) << "sec_token=\"" << i->token << "\" (len=" << i->token.size() << ") is unequal to \"" << s << "\" (len=" << s.size() << ")!";
	}
	return s == i->token;
}


void JsonAdapter::augment(json_spirit::Object& returnObject)
{
	check_guard();
	// nothing to do anymore.
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
	EventListenerValue& el = eventListener[key];  // adds an entry, if not already there. :-)
	LCK.unlock();
	
	const size_t size = el.Q.size();
	if(size)
	{
		L << Logger::Debug << size << " events in queue for key " << to_log(key) << ":";
		// fetch all elements from queue
		for(size_t i=0; i<size; ++i)
		{
			js::Object obj{ el.Q.pop_front() };
			const std::string obj_s = js::write( obj );
			L << Logger::Debug << "\t#" << i << ": " << obj_s;
			
			arr.emplace_back( std::move(obj) );
		}
	}else{
		// block until there is at least one element or timeout
		L << Logger::Debug << "Queue for key " << to_log(key) << " is empty. I'll block for " << timeout_seconds << " seconds.";
		js::Object event;
		const bool success = el.Q.try_pop_front( event, std::chrono::seconds(timeout_seconds) );
		if(success)
		{
			const std::string event_s = js::write(event);
			L << Logger::Debug << "Success! Got this event: " << event_s ;
			arr.emplace_back( std::move(event) );
		}else{
			L << Logger::Debug << "Timeout. No event after " << timeout_seconds << " seconds arreived. So sad.";
		}
	}
	
	L << Logger::Debug << "Return array with " << arr.size() << " elements.";
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


JsonAdapter& JsonAdapter::startup()
{
	JsonAdapter& ja = getInstance();
	ja.prepare_run("127.0.0.1", 4223, 9999);
	ja.run();
	return ja;
}


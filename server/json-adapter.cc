#include <stdexcept>
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <cstdint>
#include <vector>
#include <evhttp.h>

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
#include <pEp/status_to_string.hh>  // from libpEpAdapter.
#include <pEp/locked_queue.hh>

#include <boost/filesystem.hpp>
#include "json_spirit/json_spirit_writer.h"
#include "json_spirit/json_spirit_reader.h"
#include "json_spirit/json_spirit_utils.h"


namespace fs = boost::filesystem;


namespace {
    using namespace pEp::utility;


std::string BaseUrl    = "/ja/0.1/";
int SrvThreadCount     = 6;

const std::string CreateSessionUrl = BaseUrl + "createSession";
const std::string GetAllSessionsUrl = BaseUrl + "getAllSessions";
const std::string ApiRequestUrl = BaseUrl + "callFunction";

const uint64_t Guard_0 = 123456789;
const uint64_t Guard_1 = 987654321;

auto ThreadDeleter = [](std::thread* t)
{
	auto& session_registry = JsonAdapter::getSessionRegistry();
	session_registry.remove(t->get_id());
};


typedef std::unique_ptr<std::thread, decltype(ThreadDeleter)> ThreadPtr;
typedef std::vector<ThreadPtr> ThreadPool;

typedef std::recursive_mutex     Mutex;
typedef std::unique_lock<Mutex>  Lock;
Mutex  _mtx;


struct EventListenerValue
{
	utility::locked_queue<std::string> Q;
};

} // end of anonymous namespace


// *sigh* necessary because messageToSend() has no obj pointer anymore. :-(
JsonAdapter* JsonAdapter::singleton = nullptr;


struct JsonAdapter::Internal
{
	std::unique_ptr<event_base, decltype(&event_base_free)> eventBase = {nullptr, &event_base_free};
	std::unique_ptr<evhttp, decltype(&evhttp_free)> evHttp = {nullptr, &evhttp_free};
	std::unique_ptr<SessionRegistry> session_registry{};
	std::string address;
	std::string token;
	std::map<std::thread::id, EventListenerValue> eventListener;
	
	Logger      Log;
	unsigned    start_port    = 0;
	unsigned    end_port      = 0;
	unsigned    port          = 0;
	inject_sync_event_t inject_sync_event = nullptr;
	
	unsigned    request_count = 0;
	evutil_socket_t sock      = -1;
	bool        running = false;
	bool        silent  = false;
	bool        ignore_session_error = false;
	bool        deliver_html = true;
	ThreadPool  threads;
//	PEP_SESSION session = nullptr;
	
	explicit Internal()
	: Log("JAI")
	{}
	
	Internal(const Internal&) = delete;
	void operator=(const Internal&) = delete;
	
	~Internal()
	{
//		if(session)
//			pEp::call_with_lock(&release, session);
//		session=nullptr;
	}
	
	static
	void requestDone(evhttp_request* req, void* userdata)
	{
		// Hum, what is to do here?
	}

/*
	static
	PEP_STATUS deliverRequest(const std::string& uri, evhttp_connection* connection, const js::Object& request)
	{
		const std::string request_s = js::write(request, js::raw_utf8);
		evhttp_request* ereq = evhttp_request_new( &requestDone, nullptr ); // ownership goes to the connection in evhttp_make_request() below.
		evhttp_add_header(ereq->output_headers, "Content-Length", std::to_string(request_s.length()).c_str());
		auto output_buffer = evhttp_request_get_output_buffer(ereq);
		evbuffer_add(output_buffer, request_s.data(), request_s.size());
		
		const int ret = evhttp_make_request(connection, ereq, EVHTTP_REQ_POST, uri.c_str() );
		
		return (ret == 0) ? PEP_STATUS_OK : PEP_UNKNOWN_ERROR;
	}
*/
	
	void makeAndDeliverRequest(const char* function_name, const js::Array& params)
	{
		const js::Object request = make_request( function_name, params);
		const std::string request_r = js::write(request);
		
		Lock L(_mtx);
		for(auto& e : eventListener)
		{
			e.second.Q.push_back(request_r);
		}
	}
	
	
	static void addToArray(js::Array&) { /* do nothing */ }
	
	template<class T, class... Rest>
	static void addToArray(js::Array& a, const InOut<T>& in, Rest&&... rest)
	{
		a.push_back( in.to_json() );
		addToArray( a, rest... );
	}
	
	template<class... Params>
	void makeAndDeliverRequest2(const char* msg_name, Params&&... params)
	{
		js::Array param_array;
		addToArray( param_array, params...);
		makeAndDeliverRequest(msg_name, param_array);
	}

};


PEP_SESSION JsonAdapter::getSessionForThread()
{
	const auto id = std::this_thread::get_id();
	return JsonAdapter::getInstance().i->session_registry->get(id);
}


In_Pep_Session::In_Pep_Session(const js::Value& v, Context*, unsigned)
: Base( JsonAdapter::getSessionForThread() )
{}


template<>
JsonAdapter* from_json(const js::Value& /* not used */)
{
	return &JsonAdapter::getInstance();
}


template<>
In<JsonAdapter*, ParamFlag::NoInput>::In(const js::Value&, Context*, unsigned)
: value{ &JsonAdapter::getInstance() }
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
//	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	getInstance().i->makeAndDeliverRequest2("messageToSend", InOut<message*>(msg) );
	return PEP_STATUS_OK;
}


PEP_STATUS JsonAdapter::notifyHandshake(pEp_identity* self, pEp_identity* partner, sync_handshake_signal sig)
{
//	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	getInstance().i->makeAndDeliverRequest2("notifyHandshake", InOut<pEp_identity*>(self), InOut<pEp_identity*>(partner), InOut<sync_handshake_signal>(sig) );
	return PEP_STATUS_OK;
}


JsonAdapter::JsonAdapter()
: guard_0(Guard_0)
, i(new Internal{})
, guard_1(Guard_1)
{
	i->eventBase.reset(event_base_new());
	if (!i->eventBase)
		throw std::runtime_error("Failed to create new base_event.");
	
	i->evHttp.reset( evhttp_new(i->eventBase.get()) );
	if (!i->evHttp)
		throw std::runtime_error("Failed to create new evhttp.");
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


void JsonAdapter::prepare_run(const std::string& address, unsigned start_port, unsigned end_port)
{
	check_guard();
	// delayed after constructor, so virtual functions are working:
	i->session_registry.reset(new SessionRegistry(this->getMessageToSend(), this->getInjectSyncEvent()));
	i->address    = address;
	i->start_port = start_port;
	i->end_port   = end_port;
	
//	Log() << "ThreadFunc: thread id " << std::this_thread::get_id() << ". \n Registry: " << to_string( session_registry );
	
	unsigned port_ofs = 0;
try_next_port:
	auto* boundSock = evhttp_bind_socket_with_handle(i->evHttp.get(), i->address.c_str(), i->start_port + port_ofs);
	if (!boundSock)
	{
		++port_ofs;
		if(i->start_port + port_ofs > i->end_port)
		{
			throw std::runtime_error("Failed to bind server socket: "
				"No free port between " + std::to_string(i->start_port) + " and " + std::to_string(i->end_port)
				);
		}
		goto try_next_port;
	}
	
	if ((i->sock = evhttp_bound_socket_get_fd(boundSock)) == -1)
		throw std::runtime_error("Failed to get server socket for next instance.");
	
	i->port = i->start_port + port_ofs;
	i->token = create_security_token(i->address, i->port, BaseUrl);
	
	Log() << "Bound to port " << i->port << ", sec_token=\"" << i->token << "\", sock=" << i->sock << ".";
}


void JsonAdapter::threadFunc()
{
	Logger L("JA:tF");
	try
	{
		const auto id = std::this_thread::get_id();
		L << Logger::Info << " +++ Thread starts: isRun=" << i->running << ", id=" << id << ". +++";
		PEP_SESSION session = i->session_registry->get(id);
		(void)session; // not used, yet.
		
		std::unique_ptr<event_base, decltype(&event_base_free)> eventBase(event_base_new(), &event_base_free);
		if (!eventBase)
			throw std::runtime_error("Failed to create new base_event.");
		
		std::unique_ptr<evhttp, decltype(&evhttp_free)> evHttp(evhttp_new(eventBase.get()), &evhttp_free);
		if (!evHttp)
			throw std::runtime_error("Failed to create new evhttp.");
		
		evhttp_set_cb(evHttp.get(), ApiRequestUrl.c_str()    , ev_server::OnApiRequest    , this);
		
		if(i->deliver_html)
		{
			evhttp_set_cb(evHttp.get(), "/pEp_functions.js"      , ev_server::OnGetFunctions  , this);
			evhttp_set_gencb(evHttp.get(), ev_server::OnOtherRequest, nullptr);
		}
		
		if (i->sock == -1) // no port bound, yet
		{
			throw std::runtime_error("You have to call prepare_run() before run()!");
		}
		else
		{
			L << Logger::Info << "\tnow I call evhttp_accept_socket()...";
			if (evhttp_accept_socket(evHttp.get(), i->sock) == -1)
				throw std::runtime_error("Failed to accept() on server socket for new instance.");
		}
		
		while(i->running)
		{
#ifdef EVLOOP_NO_EXIT_ON_EMPTY
			// for libevent 2.1:
			event_base_loop(eventBase.get(), EVLOOP_NO_EXIT_ON_EMPTY);
#else
			// for libevent 2.0:
			event_base_loop(eventBase.get(), 0);
#endif
		}
		
		Lock L{_mtx};
		i->eventListener.erase( std::this_thread::get_id() );
	}
	catch (const std::exception& e)
	{
		L << Logger::Error << " +++ std::exception in ThreadFunc: " << e.what();
		initExcept = std::current_exception();
		Lock L{_mtx};
		i->eventListener.erase( std::this_thread::get_id() );
	}
	catch (...)
	{
		L << Logger::Crit << " +++ UNKNOWN EXCEPTION in ThreadFunc +++ ";
		initExcept = std::current_exception();
		Lock L{_mtx};
		i->eventListener.erase( std::this_thread::get_id() );
	}
	L << Logger::Info << " +++ Thread exit? isRun=" << i->running << ", id=" << std::this_thread::get_id() << ". initExcept is " << (initExcept?"":"not ") << "set. +++";
}


void JsonAdapter::run()
try
{
	check_guard();
	Logger L("JA:run");
	
	L << Logger::Info << "This is " << (void*)this << ", thread id " << std::this_thread::get_id() << ".";
	L << Logger::Debug << "Registry:\n" << i->session_registry->to_string();
	
	i->running = true;
	for(int t=0; t<SrvThreadCount; ++t)
	{
		L << Logger::Info << "Start Thread #" << t << "...";
		ThreadPtr thread(new std::thread(staticThreadFunc, this), ThreadDeleter);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
		if (initExcept)
		{
			thread->join();
			i->running = false;
			std::rethrow_exception(initExcept);
		}
		i->threads.push_back(std::move(thread));
	}
	L << Logger::Debug << "All " << SrvThreadCount << " thread(s) started:";
	for(const auto& t:i->threads)
	{
		L << Logger::Debug << "\tthread_id()=" << t->get_id() << ".";
	}
}
catch (std::exception const &e)
{
	Log(Logger::Error) << "Exception in JsonAdapter::run(): \"" << e.what() << "\"";
	throw;
}


void JsonAdapter::shutdown(timeval* t)
{
	exit(0);  // HACK for JSON-41
	check_guard();
	Log() << "JS::shutdown() was called.";
	i->running = false;
	
	/**** FIXME: proper shutdown!
	const int ret = event_base_loopexit(i->eventBase.get(), t);
	if(ret!=0)
	{
		throw std::runtime_error("JsonAdapter::shutdown() failed.");
	}
	****/
	Log() << "JS::shutdown(): event_base loop is finished.\n";
	Log() << "\t there are " << i->threads.size() << " threads remaining in the threadpool.";
	for(const auto& t : i->threads)
	{
		Log() << "\t\tjoin() on id=" << t->get_id() << "....";
		t->join();
	}
	i->threads.clear();
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
	/*
	PEP_SESSION session = this->i->session;
	auto errorstack = get_errorstack(session);
	returnObject.emplace_back( "errorstack", to_json(errorstack) );
	clear_errorstack(session);
	*/
}


js::Array JsonAdapter::pollForEvents(unsigned timeout_seconds)
{
	js::Array arr{};
	
	Lock L{_mtx};
	EventListenerValue& el = i->eventListener[ std::this_thread::get_id() ];  // adds an entry, if not already there. :-)
	L.unlock();
	
	const size_t size = el.Q.size();
	if(size)
	{
		// fetch all elements from queue
		for(size_t i=0; i<size; ++i)
		{
			arr.emplace_back( el.Q.pop_front() );
		}
	}else{
		// block until there is at least one element or timeout
		std::string event;
		const bool success = el.Q.try_pop_front( event, std::chrono::steady_clock::now() + std::chrono::seconds(timeout_seconds) );
		if(success)
		{
			arr.emplace_back( std::move(event) );
		}
	}
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


JsonAdapter& JsonAdapter::startup()
{
	JsonAdapter& ja = getInstance();
	ja.prepare_run("127.0.0.1", 4223, 9999);
	ja.run();
	return ja;
}


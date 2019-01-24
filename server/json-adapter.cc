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

#include <mutex>

#include "json-adapter.hh"
#include "daemonize.hh"
#include "pep-types.hh"
#include "json_rpc.hh"
#include "security-token.hh"
#include "pep-utils.hh"
#include "ev_server.hh"
#include "logger.hh"
#include "server_version.hh"

#include <pEp/keymanagement.h>
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
int SrvThreadCount     = 1;

const std::string CreateSessionUrl = BaseUrl + "createSession";
const std::string GetAllSessionsUrl = BaseUrl + "getAllSessions";
const std::string ApiRequestUrl = BaseUrl + "callFunction";

const uint64_t Guard_0 = 123456789;
const uint64_t Guard_1 = 987654321;

typedef std::map<std::thread::id, JsonAdapter*> SessionRegistry;

SessionRegistry session_registry;
std::string to_string(const SessionRegistry& reg);


auto ThreadDeleter = [](std::thread* t)
{
	const auto id = t->get_id();
	const auto q = session_registry.find( id );
	if(q != session_registry.end())
	{
		delete q->second;
	}
	
	delete t;
};


typedef std::unique_ptr<std::thread, decltype(ThreadDeleter)> ThreadPtr;
typedef std::vector<ThreadPtr> ThreadPool;

// keyserver lookup
utility::locked_queue< pEp_identity*, &free_identity> keyserver_lookup_queue;
PEP_SESSION keyserver_lookup_session = nullptr; // FIXME: what if another adapter started it already?
ThreadPtr   keyserver_lookup_thread{nullptr, ThreadDeleter};



std::mutex js_mutex;

// TODO: use && and std::forward<> to avoid copying of the arguments.
// It is not relevant, yet, because at the moment we use this function template only
// for init() and release() which have cheap-to-copy pointer parameters only
template<class R, class... Args>
R call_with_lock( R(*fn)(Args...), Args... args)
{
	std::lock_guard<std::mutex> L(js_mutex);
	return fn(args...);
}


// *sigh* necessary because messageToSend() has no obj pointer anymore. :-(
JsonAdapter* ja_singleton = 0;

} // end of anonymous namespace


PEP_SESSION JsonAdapter::first_session = nullptr;


typedef std::pair<std::string, unsigned> EventListenerKey;

struct EventListenerValue
{
	std::string securityContext;
	std::unique_ptr<evhttp_connection, decltype(&evhttp_connection_free)> connection = { nullptr, &evhttp_connection_free};
};



struct JsonAdapter::Internal
{
	std::unique_ptr<event_base, decltype(&event_base_free)> eventBase = {nullptr, &event_base_free};
	std::unique_ptr<evhttp, decltype(&evhttp_free)> evHttp = {nullptr, &evhttp_free};
	std::string address;
	std::string token;
	std::map<EventListenerKey, EventListenerValue> eventListener;
	
	Logger      Log;
	unsigned    start_port    = 0;
	unsigned    end_port      = 0;
	unsigned    port          = 0;
	unsigned    request_count = 0;
	evutil_socket_t sock      = -1;
	bool        shall_sync    = false; // just hold the value from config/command line.
	bool        running = false;
	bool        silent  = false;
	bool        ignore_session_error = false;
	bool        deliver_html = true;
	ThreadPool  threads;
	PEP_SESSION session = nullptr;
	
	// Sync
	utility::locked_queue< Sync_event*, &free_Sync_event>  sync_queue;
	PEP_SESSION sync_session = nullptr;
	ThreadPtr   sync_thread{nullptr, ThreadDeleter};
	
	
	explicit Internal()
	: Log("JAI")
	{}
	
	Internal(const Internal&) = delete;
	void operator=(const Internal&) = delete;
	
	~Internal()
	{
		stopSync();
		if(session)
			call_with_lock(&release, session);
		session=nullptr;
	}
	
	void stopSync();
	
	static
	void requestDone(evhttp_request* req, void* userdata)
	{
		// Hum, what is to do here?
	}

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
	
	PEP_STATUS makeAndDeliverRequest(const char* function_name, const js::Array& params)
	{
		PEP_STATUS status = PEP_STATUS_OK;
		for(auto& e : eventListener)
		{
			const js::Object request = make_request( function_name, params, e.second.securityContext );
			const std::string uri = "http://" + e.first.first + ":" + std::to_string(e.first.second) + "/";
			const PEP_STATUS s2 = deliverRequest( uri, e.second.connection.get(), request );
			if(s2!=PEP_STATUS_OK)
			{
				status = s2;
			}
		}
		return status;
	}
	
	
	static void addToArray(js::Array&) { /* do nothing */ }
	
	template<class T, class... Rest>
	static void addToArray(js::Array& a, const InOut<T>& in, Rest&&... rest)
	{
		a.push_back( in.to_json() );
		addToArray( a, rest... );
	}
	
	template<class... Params>
	PEP_STATUS makeAndDeliverRequest2(const char* msg_name, Params&&... params)
	{
		js::Array param_array;
		addToArray( param_array, params...);
		return makeAndDeliverRequest(msg_name, param_array);
	}
	
	int injectSyncMsg(Sync_event* msg)
	{
		sync_queue.push_back(msg);
		return 0;
	}
	
	int injectIdentity(pEp_identity* idy)
	{
		keyserver_lookup_queue.push_back(idy);
		return 0;
	}
	
	Sync_event* retrieveNextSyncMsg(unsigned timeout)
	{
		Sync_event* msg = nullptr;
		if(timeout)
		{
			const bool success = sync_queue.try_pop_front(msg, std::chrono::seconds(timeout));
			if(!success)
			{
				// this is timeout occurrence
				return new_sync_timeout_event();
			}
		}else{
			msg = sync_queue.pop_front();
		}
		return msg;
	}
	
	pEp_identity* retrieveNextIdentity()
	{
		return keyserver_lookup_queue.pop_front();
	}
	
	void* syncThreadRoutine(void* arg)
	{
		PEP_STATUS status = call_with_lock(&init, &sync_session, &JsonAdapter::messageToSend, &JsonAdapter::injectSyncMsg);
		if (status != PEP_STATUS_OK)
			throw std::runtime_error("Cannot init sync_session! status: " + ::pEp::status_to_string(status));
		
		status = register_sync_callbacks(sync_session,
		                                 (void*) this,
		                                 JsonAdapter::notifyHandshake,
		                                 JsonAdapter::retrieveNextSyncMsg);
		if (status != PEP_STATUS_OK)
			throw std::runtime_error("Cannot register sync callbacks! status: " + ::pEp::status_to_string(status));
		
		status = do_sync_protocol(sync_session, arg); // does the whole work
		sync_queue.clear(); // remove remaining messages
		return (void*) status;
	}
};


std::string getSessions()
{
	js::Array a;
	a.reserve(session_registry.size());
	for(const auto& s : session_registry)
	{
		std::stringstream ss;
		js::Object o;
		ss << s.first;
		o.emplace_back("tid", ss.str() );
		ss.str("");
		ss << static_cast<void*>(s.second);
		o.emplace_back("session", ss.str() );
		if(s.first == std::this_thread::get_id())
		{
			o.emplace_back("mine", true);
		}
		a.push_back( std::move(o) );
	}
	
	return js::write( a, js::pretty_print | js::raw_utf8 | js::single_line_arrays );
}


template<>
PEP_SESSION from_json(const js::Value& /* not used */)
{
	const auto id = std::this_thread::get_id();
	const auto q = session_registry.find( id );
	if(q == session_registry.end())
	{
		std::stringstream ss;
		ss << "There is no SESSION for this thread (" << id << ")!"; 
		throw std::logic_error( ss.str() );
	}else{
//		std::cerr << "from_json<PEP_SESSION> for thread " << id << " got " << (void*)q->second->i->session << ".\n";
	}
	return q->second->i->session;
}


template<>
JsonAdapter* from_json(const js::Value& /* not used */)
{
	const auto id = std::this_thread::get_id();
	const auto q = session_registry.find( id );
	if(q == session_registry.end())
	{
		std::stringstream ss;
		ss << "There is no JsonAdapter registered for this thread (" << id << ")!"; 
		throw std::logic_error( ss.str() );
	}
	return q->second;
}


template<>
In<JsonAdapter*, ParamFlag::NoInput>::~In()
{
	// nothing to do here. :-D
}

template<>
struct Type2String<In<JsonAdapter*, ParamFlag::NoInput>>
{
	static js::Value get() { throw "Make MSVC happy again. m("; }
};


ServerVersion JsonAdapter::version()
{
	return server_version();
}



PEP_STATUS JsonAdapter::messageToSend(message* msg)
{
//	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return ja_singleton->i->makeAndDeliverRequest2("messageToSend", InOut<message*>(msg) );
}


PEP_STATUS JsonAdapter::notifyHandshake(pEp_identity* self, pEp_identity* partner, sync_handshake_signal sig)
{
//	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return ja_singleton->i->makeAndDeliverRequest2("notifyHandshake", InOut<pEp_identity*>(self), InOut<pEp_identity*>(partner), InOut<sync_handshake_signal>(sig) );
}


// BEWARE: msg is 1st parameter, obj is second!!!
int JsonAdapter::injectSyncMsg(Sync_event* msg, void* obj)
{
	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return ja->i->injectSyncMsg(msg);
}


Sync_event* JsonAdapter::retrieveNextSyncMsg(void* obj, unsigned timeout)
{
	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return ja->i->retrieveNextSyncMsg(timeout);
}


void* JsonAdapter::syncThreadRoutine(void* arg)
{
	JsonAdapter* ja = static_cast<JsonAdapter*>(arg);
	return ja->i->syncThreadRoutine(arg);
}


void JsonAdapter::startSync()
{
	check_guard();
	i->sync_queue.clear();
	
	
//	status = attach_sync_session(i->session, i->sync_session);
//	if(status != PEP_STATUS_OK)
//		throw std::runtime_error("Cannot attach to sync session! status: " + status_to_string(status));
	
	i->sync_thread.reset( new std::thread( JsonAdapter::syncThreadRoutine, (void*)this ) );
}


void JsonAdapter::stopSync()
{
	check_guard();
	i->stopSync();
}


void JsonAdapter::Internal::stopSync()
{
	// No sync session active
	if(sync_session == nullptr)
		return;
	
	sync_queue.push_front(NULL);
	sync_thread->join();
	
	unregister_sync_callbacks(sync_session);
	sync_queue.clear();
	
	call_with_lock(&release, sync_session);
	sync_session = nullptr;
}


void JsonAdapter::startKeyserverLookup()
{
	if(keyserver_lookup_session)
		throw std::runtime_error("KeyserverLookup already started.");

	PEP_STATUS status = call_with_lock(&init, &keyserver_lookup_session, &JsonAdapter::messageToSend, &JsonAdapter::injectSyncMsg);
	if(status != PEP_STATUS_OK || keyserver_lookup_session==nullptr)
	{
		throw std::runtime_error("Cannot create keyserver lookup session! status: " + ::pEp::status_to_string(status));
	}
	
	keyserver_lookup_queue.clear();
	status = register_examine_function(keyserver_lookup_session,
			JsonAdapter::examineIdentity,
			&keyserver_lookup_session // nullptr is not accepted, so any dummy ptr is used here
			);
	if (status != PEP_STATUS_OK)
		throw std::runtime_error("Cannot register keyserver lookup callbacks! status: " + ::pEp::status_to_string(status));
	
	keyserver_lookup_thread.reset( new std::thread( JsonAdapter::keyserverLookupThreadRoutine, &keyserver_lookup_session /* just a dummy */ ) );
}


void JsonAdapter::stopKeyserverLookup()
{
	// No keyserver lookup session active
	if(keyserver_lookup_session == nullptr)
		return;
	
	keyserver_lookup_queue.push_front(NULL);
	keyserver_lookup_thread->join();

	// there is no unregister_examine_callback() function. hum...
	keyserver_lookup_queue.clear();
	call_with_lock(&release, keyserver_lookup_session);
	keyserver_lookup_session = nullptr;
}


int JsonAdapter::examineIdentity(pEp_identity* idy, void* obj)
{
	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return ja->i->injectIdentity(idy);
}


pEp_identity* JsonAdapter::retrieveNextIdentity(void* obj)
{
	JsonAdapter* ja = static_cast<JsonAdapter*>(obj);
	return ja->i->retrieveNextIdentity();
}


void* JsonAdapter::keyserverLookupThreadRoutine(void* arg)
{
	PEP_STATUS status = do_keymanagement(
		&JsonAdapter::retrieveNextIdentity,
		arg); // does the whole work
	
	keyserver_lookup_queue.clear();
	return (void*) status;
}


JsonAdapter::JsonAdapter()
: guard_0(Guard_0)
, i(new Internal{})
, guard_1(Guard_1)
{
	if(!ja_singleton)
	{
		ja_singleton = this;
	}
	
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
	Log() << "~JsonAdapter(): " << session_registry.size() << " sessions registered.";
	stopSync();
	this->shutdown(nullptr);
	Log() << "\t After stopSync() and shutdown() there are " << session_registry.size() << " sessions registered.";
	delete i;
	i=nullptr;
}


JsonAdapter& JsonAdapter::do_sync(bool _do_sync)
{
	check_guard();
	i->shall_sync = _do_sync;
	return *this;
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
	i->address    = address;
	i->start_port = start_port;
	i->end_port   = end_port;
	
	if(first_session == nullptr) // okay, we are the 1st:
	{
	// create a dummy session just to see whether the Engine is functional.
	// reason: here we still can log errors to stderr, because prepare_run() is called before daemonize().
	PEP_STATUS status = call_with_lock(&init, &first_session, &JsonAdapter::messageToSend, &JsonAdapter::injectSyncMsg);
	if(status != PEP_STATUS_OK || first_session==nullptr)
	{
		const std::string error_msg = "Cannot create first session! PEP_STATUS: " + ::pEp::status_to_string(status) + ".";
		std::cerr << error_msg << std::endl; // Log to stderr intentionally, so Enigmail can grab that error message easily.
		if( ! i->ignore_session_error)
		{
			throw std::runtime_error(error_msg);
		}
	}
	}
	
	Log() << "ThreadFunc: thread id " << std::this_thread::get_id() << ". \n Registry: " << to_string( session_registry );
	
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
	
	Log() << "Bound to port " << i->port << ", sec_token=\"" << i->token << "\"";
}


void JsonAdapter::threadFunc()
{
	Logger L("JA:tF");
	try
	{
		const auto id = std::this_thread::get_id();
		L << Logger::Info << " +++ Thread starts: isRun=" << i->running << ", id=" << id << ". +++";
		const auto q=session_registry.find(id);
		if(q==session_registry.end())
		{
			i->session = nullptr;
			PEP_STATUS status = call_with_lock(&init, &i->session, &JsonAdapter::messageToSend, &JsonAdapter::injectSyncMsg); // release(session) in ThreadDeleter
			if(status != PEP_STATUS_OK || i->session==nullptr)
			{
				const std::string error_msg = "Cannot create session! PEP_STATUS: " + ::pEp::status_to_string(status) + ".";
				L << Logger::Error << error_msg;
				if( ! i->ignore_session_error)
				{
					throw std::runtime_error(error_msg);
				}
			}
			
			session_registry.emplace(id, this);
			L << Logger::Info << "\tcreated new session for this thread: " << static_cast<void*>(i->session) << ".";
			if(i->shall_sync && i->session) // startSync() does not make sense without session.
			{
				L << Logger::Info << "\tstartSync()...";
				startSync();
			}
		}else{
			L << Logger::Info << "\tsession for this thread: "  << static_cast<void*>(q->second) << ".";
		}
		
		std::unique_ptr<event_base, decltype(&event_base_free)> eventBase(event_base_new(), &event_base_free);
		if (!eventBase)
			throw std::runtime_error("Failed to create new base_event.");
		
		std::unique_ptr<evhttp, decltype(&evhttp_free)> evHttp(evhttp_new(eventBase.get()), &evhttp_free);
		if (!evHttp)
			throw std::runtime_error("Failed to create new evhttp.");
		
		evhttp_set_cb(evHttp.get(), ApiRequestUrl.c_str()    , ev_server::OnApiRequest    , this);
		
		if(i->deliver_html)
		{
			evhttp_set_cb(evHttp.get(), "/pep_functions.js"      , ev_server::OnGetFunctions  , this);
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
	}
	catch (const std::exception& e)
	{
		L << Logger::Error << " +++ std::exception in ThreadFunc: " << e.what();
		initExcept = std::current_exception();
	}
	catch (...)
	{
		L << Logger::Crit << " +++ UNKNOWN EXCEPTION in ThreadFunc +++ ";
		initExcept = std::current_exception();
	}
	L << Logger::Info << " +++ Thread exit? isRun=" << i->running << ", id=" << std::this_thread::get_id() << ". initExcept is " << (initExcept?"":"not ") << "set. +++";
}


void JsonAdapter::run()
try
{
	check_guard();
	Logger L("JA:run");
	
	L << Logger::Info << "This is " << (void*)this << ", thread id " << std::this_thread::get_id() << ".";
	L << Logger::Debug << to_string( session_registry);
	
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
	PEP_SESSION session = this->i->session;
	auto errorstack = get_errorstack(session);
	returnObject.emplace_back( "errorstack", to_json(errorstack) );
	clear_errorstack(session);
}


void JsonAdapter::registerEventListener(const std::string& address, unsigned port, const std::string& securityContext)
{
	check_guard();
	const auto key = std::make_pair(address, port);
	const auto q = i->eventListener.find(key);
	if( q != i->eventListener.end() && q->second.securityContext != securityContext)
	{
		throw std::runtime_error("EventListener at host \"" + address + "\":" + std::to_string(port) + " is already registered with different securityContext." );
	}
	
	EventListenerValue v;
	v.securityContext = securityContext;
// FIXME: one event_base per thread!
//	v.connection.reset( evhttp_connection_base_new( i->eventBase.get(), nullptr, address.c_str(), port ) );
	i->eventListener[key] = std::move(v);
}


void JsonAdapter::unregisterEventListener(const std::string& address, unsigned port, const std::string& securityContext)
{
	check_guard();
	const auto key = std::make_pair(address, port);
	const auto q = i->eventListener.find(key);
	if( q == i->eventListener.end() || q->second.securityContext != securityContext)
	{
		throw std::runtime_error("Cannot unregister EventListener at host \"" + address + "\":" + std::to_string(port) + ". Not registered or wrong securityContext." );
	}
	
	i->eventListener.erase(q);
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
	if(guard_0 != Guard_0 || guard_1 != Guard_1)
	{
		char buf[128];
		snprintf(buf,127, "JS::check_guard failed: guard0=%llu, guard1=%llu this=%p.\n",
			guard_0, guard_1, (void*)this
			);
		std::cerr << buf; // Log() might not work here, when memory is corrupted
		throw std::logic_error( buf );
	}
}


void JsonAdapter::global_shutdown()
{
	call_with_lock(&release, JsonAdapter::first_session);
	JsonAdapter::first_session = nullptr;
}


namespace {

std::string to_string(const SessionRegistry& reg)
{
	std::stringstream ss;
	ss << "There are " << reg.size() << " sessions registered" << (reg.empty() ? '.' : ':' ) << std::endl;
	for(const auto s : reg)
	{
		ss << "\t thread id " << s.first << " : JA=" << (void*)s.second << ". ";
		if(s.second)
		{
			ss << " js.i=" << (void*)(s.second->i) << ". ";
			if(s.second->i)
			{
				ss << "  session=" << (void*)(s.second->i->session) << ".";
			}
		}
		ss << std::endl;
	}
	return ss.str();
}

}

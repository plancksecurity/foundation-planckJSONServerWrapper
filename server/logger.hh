#ifndef LOGGER_HH
#define LOGGER_HH

#include <cstdio>
#include <string>
#include <cerrno>
#include <sys/time.h>


#ifdef DEBUG_ENABLED
#define DEBUG_OUT( LL , ... )  LL.debug( __VA_ARGS__ )
#define DEBUGI_OUT( LL , ... )  LL.debugInternal( __VA_ARGS__ )
#else
#define DEBUG_OUT(...)  do{}while(0)
#define DEBUGI_OUT(...)  do{}while(0)
#endif

#ifdef __GNUC__
#define PRINTF __attribute__((format (printf, 2,3) ))
#define PRINTF3 __attribute__((format (printf, 3,4) ))
#else
#define PRINTF
#define PRINTF3
#endif


class Logger;

// get global Logger instance
Logger& getLogger();

class Logger
{
public:
	enum Severity
	{
		Emergency,// The message says the system is unusable.
		Alert,    // Action on the message must be taken immediately.
		Crit,     // The message states a critical condition.
		Error,    // The message describes an error.
		Warn,     // The message is a warning.
		Notice,   // The message describes a normal but important event.
		Info,     // The message is purely informational.
		Debug,    // The message is only for interface debugging purposes.
		DebugInternal, // The message is for internal debugging purposes.
		
		Inherited=-1 // Use the same loglevel as the parent Logger (if any)
	};

	enum Target
	{
		Console=1, Syslog=2, File=4  // may be ORed together
	};

	// shall be called before first log message.
	static void start(const std::string& program_name);

	// returns a string in YYYY-MM-DD.hh:mm:ss format of the given time_t t
	static std::string gmtime(time_t t);

	// returns a string in YYYY-MM-DD.hh:mm:ss.uuuuuu format (u=microseconds) of the given timval
	static std::string gmtime(timeval t);

	void emergency(const char* format, ...) PRINTF;
	void emergency(const std::string& line);

	void alert(const char* format, ...) PRINTF;
	void alert(const std::string& line);

	void critical(const char* format, ...) PRINTF;
	void critical(const std::string& line);

	void error(const char* format, ...) PRINTF;
	void error(const std::string& line);

	void warning(const char* format, ...) PRINTF;
	void warning(const std::string& line);

	void notice(const char* format, ...) PRINTF;
	void notice(const std::string& line);

	void info(const char* format, ...) PRINTF;
	void info(const std::string& line);

#ifdef DEBUG_ENABLED
	void debug(const char* format, ...) PRINTF;
	void debug(const std::string& line);

	void debugInternal(const char* format, ...) PRINTF;
	void debugInternal(const std::string& line);
#else
	void debug(const char*, ...) PRINTF  { /* do nothing */ }
	void debug(const std::string&) { /* do nothing */ }

	void debugInternal(const char*, ...) PRINTF { /* do nothing */ }
	void debugInternal(const std::string&) { /* do nothing */ }
#endif

	void log(Severity s, const char* format, ...) PRINTF3; // C style
	void log(Severity s, const std::string& line); // C++ style :-)

	// log messages with a lower severity are ignored
	void setLevel(Severity s);
	Severity getLevel() const;

	static void setTarget(Target t);
	static Target getTarget();
	
	const std::string& getPrefix() const;

	explicit Logger(const std::string& my_prefix, Severity my_severity);
	explicit Logger(Logger& parent, const std::string& my_prefix, Severity my_severity = Severity::Inherited);

	// non-copyable:
	Logger(const Logger&) = delete;
	void operator=(const Logger&) = delete;

#ifdef DEBUG_ENABLED
	class Stream
	{
		Stream(Logger*);
		Stream(Stream& parent, const std::string& msg);
		
		Logger* L;
		Stream* parent;
		std::string s;
	public:
		~Stream();
		friend class Logger;

		template<class T> 
		friend Logger::Stream operator<<(Logger::Stream, const T&);

		friend Logger::Stream operator<<(Logger::Stream, const char*const);
	};

	operator Stream();
#else
	template<class T>
	Logger& operator<<(const T&) { return *this; }
#endif

private:
	friend Logger& getLogger();
	const std::string prefix;
	Severity loglevel;
};


#ifdef DEBUG_ENABLED
	template<class T>
	Logger::Stream operator<<(Logger::Stream, const T&);
	Logger::Stream operator<<(Logger::Stream, const char*const);
#endif

// clean up defines which may collide with other headers...
#undef PRINTF
#undef PRINTF3

#endif // LOGGER_HH

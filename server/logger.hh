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
		None = 0,
		Console=1, Syslog=2, File=4  // may be ORed together
	};

	// shall be called before first log message.
	static void start(const std::string& program_name, const std::string& filename = "");

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

	static void setDefaultLevel(Severity s);
	static Severity getDefaultLevel();

	static void setDefaultTarget(Target t);
	static Target getDefaultTarget();

	// set maximum length of a log message. Longer messages will be clipped!
	static void setMaxMessageLength(unsigned length);
	
	// set maximum length of a log _line_. Longer messages will be wrapped into several lines.
	static void setMaxLineLength(unsigned length);

	static unsigned getMaxMessageLength();
	static unsigned getMaxLineLength();

	const std::string& getPrefix() const;

	// if no explicit severity is given it is taken from default's severity
	explicit Logger(const std::string& my_prefix, Severity my_severity = Severity::Inherited);
	explicit Logger(Logger& parent, const std::string& my_prefix, Severity my_severity = Severity::Inherited);

	// non-copyable:
	Logger(const Logger&) = delete;
	void operator=(const Logger&) = delete;


	class Stream
	{
	public:
		Stream(Logger* _L, Logger::Severity _sev);
		~Stream();
		
		mutable std::string s;
		
	private:
		Logger* L;
		const Logger::Severity sev;
	};
	
;
private:
	friend Logger& getLogger();
	const std::string prefix;
	Severity loglevel;
};

	// creates a Stream, who collect data in pieces and logs it to the "parent" logger in its destructor with the given severity
	Logger::Stream operator<<(Logger& parent, Logger::Severity sev);

	template<class T>
	const Logger::Stream& operator<<(const Logger::Stream&, const T&);
	const Logger::Stream& operator<<(const Logger::Stream&, const char*const);

	inline
	const Logger::Stream& operator<<(const Logger::Stream& stream, char*const s)
	{
		return stream << const_cast<const char*>(s);
	}

// clean up defines which may collide with other headers...
#undef PRINTF
#undef PRINTF3

#endif // LOGGER_HH


#include "logger.hh"
#include "logger_config.hh"

#include <cstdarg>
#include <ctime>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <sstream>
#include <sys/time.h>

#ifdef LOGGER_ENABLE_SYSLOG
extern "C" {
#include <syslog.h>
}
#endif // LOGGER_ENABLE_SYSLOG

using Lock = std::lock_guard<std::recursive_mutex>;


enum { LogLineMax = LOGGER_MAX_LOG_LINE_LENGTH };

namespace LoggerS  // namespace containing all data for the Logger singleton. HACK!
{
	std::FILE* logfile = 0;
	std::recursive_mutex mut;
	Logger::Severity loglevel = Logger::DebugInternal;
	Logger::Target target = Logger::Target(-1);
	std::string filename;
	std::string ident;
	
	bool initialized = false;
	
	bool omit_timestamp = false;

	void openfile();
	void opensyslog();

	void start(const std::string& program_name, const std::string& filename = std::string())
	{
		ident = program_name;
		// TODO: use $TEMP, $TMP etc.
		LoggerS::filename = filename.empty() ? "/tmp/log-" + program_name + ".log" : filename;
		opensyslog();
		openfile();
		initialized = true;
	}

	void log(Logger::Severity s, const std::string& msg);

#ifdef LOGGER_USE_ASCII_TAGS
	const char* Levelname[] =
		{
			"<<EMERGENCY>>",
			"<<ALERT>>",
			"<<CRITICAL>>",
			"<ERROR>",
			"<Warning>",
			"<Notice>",
			"<info>",
			"<>",  // Debug
			"! "   // DebugInternal
		};
	
	const char* multiline[] =
		{
			"⎡", "⎢", "⎣", "▒"
		};
#else
	const char* Levelname[] =
		{
			"»»EMERGENCY",
			"»»ALERT",
			"»»CRITICAL",
			"»ERROR",
			"»Warning",
			"»Notice",
			"»Info",
			"°",  // Debug
			"·"   // DebugInternal
		};
	
	const char* multiline[] =
		{
			"/ ", "| ", "\\ ", "##"
		};
#endif

} // end of namespace LoggerS


void Logger::start(const std::string& program_name, const std::string& filename)
{
	if(LoggerS::initialized==false)
	{
		LoggerS::start(program_name, filename);
		Logger& l = ::getLogger();
		l.debug("Logger has been started.");
	}
}


std::string Logger::gmtime(time_t t)
{
	char buf[24]; // long enough to hold YYYYY-MM-DD.hh:mm:ss" (y10k-safe!)
	std::tm     T;
	gmtime_r(&t, &T); // TODO: GNU extension also in std:: ?
	std::snprintf(buf, sizeof(buf)-1, "%04d-%02d-%02d.%02d:%02d:%02d",
		T.tm_year+1900, T.tm_mon+1, T.tm_mday, T.tm_hour, T.tm_min, T.tm_sec );
	
	return buf;
}


std::string Logger::gmtime(timeval t)
{
	char buf[31]; // long enough to hold YYYYY-MM-DD.hh:mm:ss.uuuuuu
	std::tm T;
	gmtime_r(&t.tv_sec, &T);  // TODO: GNU extension also in std:: ?
	std::snprintf(buf, sizeof(buf)-1, "%04d-%02d-%02d.%02d:%02d:%02d.%06lu",
	         T.tm_year+1900, T.tm_mon+1, T.tm_mday, T.tm_hour, T.tm_min, T.tm_sec, (long unsigned)t.tv_usec);
	
	return buf;
}


Logger::Severity Logger::getLevel() const
{
	return loglevel;
}


void Logger::setLevel(Severity s)
{
	// TODO: use C++17 std::clamp()
	loglevel = s<Emergency ? Emergency : (s>DebugInternal ? DebugInternal : s);
}


const std::string& Logger::getPrefix() const
{
	return prefix;
}


Logger::Target Logger::getDefaultTarget()
{
	return LoggerS::target;
}


void Logger::setDefaultTarget(Target t)
{
	LoggerS::target = t;
}


Logger::Severity Logger::getDefaultLevel()
{
	return LoggerS::loglevel;
}


void Logger::setDefaultLevel(Severity s)
{
	LoggerS::loglevel = s;
}


Logger::Logger(const std::string& my_prefix, Severity my_loglevel)
: prefix(my_prefix + ":")
{
	setLevel(my_loglevel == Severity::Inherited ? getDefaultLevel() : my_loglevel);
	start(my_prefix); // if not yet initialized.
}


Logger::Logger(Logger& parent, const std::string& my_prefix, Severity my_loglevel)
: Logger( parent.getPrefix() + my_prefix + ':', my_loglevel == Severity::Inherited ? parent.getLevel() : my_loglevel )
{
}


void Logger::log(Severity s, const char* format, ...)
{
	if(s<=loglevel && s<=LoggerS::loglevel)
	{
		va_list va;
		va_start(va, format);
		char buf[ LogLineMax + 1];
		std::vsnprintf(buf, LogLineMax, format, va);
		va_end(va);
		
		LoggerS::log(s, prefix + buf);
	}
}


void LogP(Logger::Severity s, Logger::Severity my_loglevel, const std::string& prefix, const char* format, va_list va)
{
	if(s<=my_loglevel && s<=LoggerS::loglevel)
	{
		char buf[ LogLineMax + 1];
		std::vsnprintf(buf, LogLineMax, format, va);
		LoggerS::log(s, prefix + buf );
	}
}


void Logger::log(Severity s, const std::string& logline)
{
	if(s<=loglevel && s<=LoggerS::loglevel)
	{
		LoggerS::log(s, prefix + logline);
	}
}


#define LOGGER_LAZY( fnname, severity ) \
	void Logger::fnname ( const std::string& line) { \
		log( Logger:: severity, line ); \
	} \
	void Logger:: fnname (const char* format, ...) { \
		va_list va;	va_start(va, format); \
		LogP( severity, loglevel, prefix, format, va ) ;\
		va_end(va); \
	} \


LOGGER_LAZY( emergency, Emergency)
LOGGER_LAZY( alert    , Alert    )
LOGGER_LAZY( critical , Crit     )
LOGGER_LAZY( error    , Error    )
LOGGER_LAZY( warning  , Warn     )
LOGGER_LAZY( notice   , Notice   )
LOGGER_LAZY( info     , Info     )

#ifdef DEBUG_ENABLED
LOGGER_LAZY( debug    , Debug    )
LOGGER_LAZY( debugInternal , DebugInternal )
#else
// intentionally left blank.
// the methods are already defined as do-nothing inline functions in Logger.hh
#endif

#undef LOGGER_LAZY


Logger& getLogger()
{
	Lock LCK(LoggerS::mut);
	static Logger log("#", Logger::Debug);
	return log;
}


// ---------- LoggerS ----------

void LoggerS::openfile()
{
	logfile = std::fopen(filename.c_str(), "a");
	if(logfile==0)
	{
		perror("Could not open log file! ");
	}
}



void LoggerS::opensyslog()
{
#ifdef LOGGER_ENABLE_SYSLOG
	openlog(ident.c_str(), 0, LOG_DAEMON);
#endif
}


static const std::string thread_alphabet = "0123456789_ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const unsigned thread_alph_len = thread_alphabet.size(); // shall be a prime number
static std::hash<std::thread::id> hash_tid;

// create a three-digit base37 string of the current thread ID for easy grepping and filtering:
std::string thread_id()
{
	char buf[8] = { ' ', '\302', '\266', '?', '?', '?', ' ' };
	unsigned long long id = hash_tid(std::this_thread::get_id());
	buf[3] = thread_alphabet.at( id % thread_alph_len); id /= thread_alph_len;
	buf[4] = thread_alphabet.at( id % thread_alph_len); id /= thread_alph_len;
	buf[5] = thread_alphabet.at( id % thread_alph_len); id /= thread_alph_len;
	return std::string(buf, buf+7);
}

void LoggerS::log(Logger::Severity s, const std::string& logline)
{
	Lock LCK(mut);
	if(s<Logger::Emergency     ) s = Logger::Emergency;
	if(s>Logger::DebugInternal ) s = Logger::DebugInternal;
	
	if(target & (Logger::File | Logger::Console))
	{
		std::string timestamp;
		timestamp.reserve(45);
		
		if(omit_timestamp == false)
		{
			timestamp = Logger::gmtime(time(0));
		}
		
		timestamp.append( thread_id() );
		timestamp.append(Levelname[s]);
		timestamp.append(" :");
		
		if(target & Logger::Console)
		{
			std::fputs(timestamp.c_str(), stderr);
			std::fputs(logline.c_str(), stderr);
			std::fputc('\n', stderr);
		}
		if(target & Logger::File)
		{
			std::fputs(timestamp.c_str(), logfile);
			std::fputs(logline.c_str(), logfile);
			std::fputc('\n', logfile);
			std::fflush(logfile);
		}
	}
	
#ifdef LOGGER_ENABLE_SYSLOG
	if(target & Logger::Syslog)
	{
		syslog(s, "%s", logline.c_str());
	}
#endif
}


Logger::Stream::Stream(Logger* parent, Severity _sev)
: L(parent), sev(_sev)
{}


Logger::Stream::~Stream()
{
	if(L)
	{
		L->log(sev, s);
	}
}


template<class T>
const Logger::Stream& operator<<(const Logger::Stream& stream, const T& t)
{
	std::stringstream ss;
	ss << t;
	stream.s.append( ss.str());
	return stream;
}


const Logger::Stream& operator<<(const Logger::Stream& stream, const char*const t)
{
	stream.s.append( t );
	return stream;
}

template<>
const Logger::Stream& operator<<(const Logger::Stream& stream, const bool& b)
{
	stream.s.append( b ? "true" : "false");
	return stream;
}

template const Logger::Stream& operator<<(const Logger::Stream&, const int&);
template const Logger::Stream& operator<<(const Logger::Stream&, const long&);
template const Logger::Stream& operator<<(const Logger::Stream&, const unsigned&);
template const Logger::Stream& operator<<(const Logger::Stream&, const unsigned long&);

template const Logger::Stream& operator<<(const Logger::Stream&, const std::string&);
template const Logger::Stream& operator<<(const Logger::Stream&, const double&);

template const Logger::Stream& operator<<(const Logger::Stream&, const void*const&);
template const Logger::Stream& operator<<(const Logger::Stream&,       void*const&);
template const Logger::Stream& operator<<(const Logger::Stream&, const std::thread::id&);

Logger::Stream operator<<(Logger& parent, Logger::Severity sev)
{
	return Logger::Stream(&parent, sev);
}

// End of file

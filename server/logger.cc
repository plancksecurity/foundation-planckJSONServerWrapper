#include "logger.hh"
#include "logger_config.hh"

#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <ctime>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <sstream>
#include <vector>
#include <sys/time.h>

#ifdef LOGGER_ENABLE_SYSLOG
extern "C" {
#include <syslog.h>
}
#endif // LOGGER_ENABLE_SYSLOG

using Lock = std::lock_guard<std::recursive_mutex>;



namespace LoggerS  // namespace containing all data for the Logger singleton. HACK!
{
	std::FILE* logfile = 0;
	std::recursive_mutex mut;
	Logger::Severity loglevel = Logger::DebugInternal;
	Logger::Target target = Logger::Target(-1);
	std::string filename;
	std::string ident;
	
	bool initialized = false;
	
	// config variables
	bool omit_timestamp = false;
	unsigned max_message_length = LOGGER_MAX_LOG_MESSAGE_LENGTH;
	unsigned max_line_length    = LOGGER_MAX_LINE_LENGTH;

	void openfile();
	void opensyslog();

	void start(const std::string& program_name, const std::string& filename = std::string())
	{
		ident = program_name;
		// TODO: use $TEMP, $TMP etc.
		opensyslog();
		if(target & Logger::Target::File)
		{
			LoggerS::filename = filename.empty() ? "/tmp/log-" + program_name + ".log" : filename;
			openfile();
		}
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
	
	const char* const MultilineBracketFirst = "/";
	const char* const MultilineBracketMid   = "|";
	const char* const MultilineBracketLast  = "\\";
	const char* const MultilineWrap         = "|>";
	const char* const MultilineWrapContinue = "<|";

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
	
	const char* const MultilineBracketFirst = "⎡ ";
	const char* const MultilineBracketMid   = "⎢ ";
	const char* const MultilineBracketLast  = "⎣ ";
	const char* const MultilineWrap         = "↩";
	const char* const MultilineWrapContinue = "↪";

#endif

	struct LogfileCloser
	{
		~LogfileCloser()
		{
			if(LoggerS::logfile)
			{
				LoggerS::log(Logger::Debug, "Shutdown.");
				fputs("---<shutdown>---\n", LoggerS::logfile);
				fclose(LoggerS::logfile);
				LoggerS::logfile = nullptr;
			}
		}
	};
	
	// guess what.
	static LogfileCloser logfile_closer;

} // end of namespace LoggerS


namespace {

	void logSingleLine(FILE* logfile, const std::string& timestamp, const std::string& logline)
	{
		std::fputs(timestamp.c_str(), logfile);
		std::fputs(logline.c_str(), logfile);
		std::fputc('\n', logfile);
	}

	void logMultiLine(FILE* logfile, const std::string& prefix, const std::vector<std::string>& lines)
	{
		assert( lines.size()>1 );
		
		// be robust, anyway. So:
		if(lines.empty())
		{
			return;
		}
		
		if(lines.size()==1)
		{
			logSingleLine(logfile, prefix, lines.at(0));
			return;
		}
		
		const size_t  last_line = lines.size()-1;
		
		std::fputs(prefix.c_str(), logfile);
		std::fputs(LoggerS::MultilineBracketFirst, logfile);
		std::fputs(lines[0].c_str(), logfile);
		std::fputc('\n', logfile);
		
		for(size_t q=1; q<last_line; ++q)
		{
			std::fputs(prefix.c_str(), logfile);
			std::fputs(LoggerS::MultilineBracketMid, logfile);
			std::fputs(lines[q].c_str(), logfile);
			std::fputc('\n', logfile);
		}
		
		std::fputs(prefix.c_str(), logfile);
		std::fputs(LoggerS::MultilineBracketLast, logfile);
		std::fputs(lines[last_line].c_str(), logfile);
		std::fputc('\n', logfile);
	}

	// wrap an overlong line into several lines, incl. adding wrapping markers
	void wrapLongLine(const std::string& oneline, std::vector<std::string>& lines)
	{
		std::string::const_iterator begin = oneline.begin();
		std::string::const_iterator end   = oneline.begin();
		std::size_t ofs = 0;
		
		bool cont_line = false;
		do{
			begin=end;
			const unsigned delta = std::min( (size_t)Logger::getMaxLineLength(), oneline.size() - ofs );
			end += delta;
			ofs += delta;
			
			if(end != oneline.end())
			{
				while( (uint8_t(*end) >= 0x80) && (end>begin) )
				{
					// rewind
					--end;
					--ofs;
				}
			}
			
			lines.push_back(
					(cont_line ? LoggerS::MultilineWrapContinue : "") +
					std::string(begin, end) +
					(end!=oneline.end() ? LoggerS::MultilineWrap : "")
				);
			cont_line = true;
		}while( end != oneline.end() );
	}
	
} // end of anonymous namespace

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

// Win32 does not have gmtime_r(), but its gmtime() returns ptr to thread-local struct tm. :-)
#ifdef _WIN32
	std::tm* T = gmtime(&t);
#else
	std::tm     myT;
	gmtime_r(&t, &myT); // TODO: GNU extension, conform to POSIX.1; works on Linux and MacOS.
	std::tm* T = &myT;
#endif

	std::snprintf(buf, sizeof(buf)-1, "%04d-%02d-%02d.%02d:%02d:%02d",
		T->tm_year+1900, T->tm_mon+1, T->tm_mday, T->tm_hour, T->tm_min, T->tm_sec );
	
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


// set maximum length of a log message. Longer messages will be clipped!
void Logger::setMaxMessageLength(unsigned length)
{
	LoggerS::max_message_length = length;
}

// set maximum length of a log _line_. Longer messages will be wrapped into several lines.
void Logger::setMaxLineLength(unsigned length)
{
	LoggerS::max_line_length = std::min(length, 64u*1024u);
}

unsigned Logger::getMaxMessageLength()
{
	return LoggerS::max_message_length;
}

unsigned Logger::getMaxLineLength()
{
	return LoggerS::max_line_length;
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
		char buf[ LoggerS::max_line_length + 1];
		std::vsnprintf(buf, LoggerS::max_line_length, format, va);
		va_end(va);
		
		LoggerS::log(s, prefix + buf);
	}
}


void LogP(Logger::Severity s, Logger::Severity my_loglevel, const std::string& prefix, const char* format, va_list va)
{
	if(s<=my_loglevel && s<=LoggerS::loglevel)
	{
		char buf[ LoggerS::max_line_length + 1];
		std::vsnprintf(buf, LoggerS::max_line_length, format, va);
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
	
	// clipt and wrap:
	bool multiline = false;
	std::vector<std::string> lines;
	std::stringstream ss(logline);
	std::string oneline;
	while(std::getline(ss, oneline))
	{
		if(oneline.size() > max_line_length)
		{
			wrapLongLine(oneline, lines);
		}else{
			lines.push_back( std::move(oneline) );
		}
	}
	
	if(lines.size() > 1)
		multiline = true;
	
	// create header with timestamp
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
			if(multiline)
			{
				logMultiLine(stderr, timestamp, lines);
			}else{
				logSingleLine(stderr, timestamp, logline);
			}
		}
		if(target & Logger::File)
		{
			if(multiline)
			{
				logMultiLine(logfile, timestamp, lines);
			}else{
				logSingleLine(logfile, timestamp, logline);
			}
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

#include <gtest/gtest.h>
#include "logger.hh"

namespace
{
	const std::string nonAscii = "€§";

	std::string longstring(unsigned length)
	{
		std::string s;
		s.reserve(length);
		unsigned u=0;
		for(; u+nonAscii.size()<length; u+=nonAscii.size())
		{
			s += nonAscii;
		}
		
		for(u=s.size(); u<length; ++u)
		{
			s += char( (u%94) + '!' );
		}
		
		
		return s;
	}
}


class LoggerTest : public ::testing::Test
{
	void SetUp()
	{
		Logger::start("unittest_logger");
	}
	
	void TearDown()
	{
		
	}
};


TEST_F( LoggerTest, Severities )
{
	Logger L("Severities");
	
	L.debug    ("This is debug");
	L.info     ("This is info");
	L.notice   ("This is notice");
	L.warning  ("This is warning");
	L.error    ("This is error");
	L.critical ("This is critical");
	L.alert    ("This is alert");
	L.emergency("This is emergency");
	
	L << Logger::Debug << "Debug via stream";
	L << Logger::Info      << "Info via stream";
	L << Logger::Notice    << "Notice via stream";
	L << Logger::Warn      << "Warning via stream";
	L << Logger::Error     << "Error via stream";
	L << Logger::Crit      << "Critical via stream";
	L << Logger::Alert     << "Alert via stream";
	L << Logger::Emergency << "Emergency via stream";
}

TEST_F( LoggerTest, LongLine )
{
	Logger L("LongLine");
	
	char buffer[64];
	double length = 10.0;
	while(length < 4444)
	{
		snprintf(buffer, 63, "Length %d octets: ", int(length));
		L.notice( buffer + longstring(length));
		++length;
	}

}

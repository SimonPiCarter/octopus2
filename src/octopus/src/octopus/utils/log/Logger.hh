#ifndef __LOGGER__
#define __LOGGER__

#include <mutex>
#include <sstream>
#include <iostream>
#include <fstream>

namespace octopus
{

	/// @brief Logger handler used to log
	/// it handles mutex
	class LoggerHandler : public std::ostringstream
	{
	public:
		explicit LoggerHandler(std::mutex &mutex_p, std::ofstream &logFile_p, bool shouldLog_p);
		LoggerHandler(LoggerHandler const &other_p);

		~LoggerHandler();

	private:
		std::mutex &_mutex;
		bool _shouldLog;
		std::ofstream &_logFile;
	};

	class LoggerVoider {
	public:
		explicit LoggerVoider(std::mutex &, std::ofstream &, bool) {}
		LoggerVoider(LoggerVoider const &) {}
	};

	/// @brief use this if you want to be able to switch from debug to normal without rebuilding (slower at execution)
	// typedef LoggerHandler DebugLoggerHandler;

	/// @brief use this if you want fastest execution (but debug log wont be possible)
	typedef LoggerVoider DebugLoggerHandler;

	class Logger
	{
	public:
		static LoggerHandler getNormal();
		static DebugLoggerHandler getDebug();

		static void enable_debug() { _debug = true; }
		static void disable_debug() { _debug = false; }

	private:
		static bool _debug;
		static std::mutex _mutex;
		static std::ofstream _logFile;
	};
}

template<typename T>
octopus::LoggerVoider operator<<(octopus::LoggerVoider voider_p, T)
{
	return voider_p;
}

// to handle std::endl
octopus::LoggerVoider operator<<(octopus::LoggerVoider voider_p, std::ostream& (*pf)(std::ostream&));

#endif

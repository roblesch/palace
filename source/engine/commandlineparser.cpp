#include "commandlineparser.hpp"

CommandLineParser::CommandLineParser()
	: PROP(m_prop)
{
}

CommandLineParser::CommandLineParser(const int argc, const char* argv[])
	: CommandLineParser()
{
}

CommandLineParser& CommandLineParser::operator=(CommandLineParser rhs)
{
	m_argc = rhs.m_argc;
	m_argv = rhs.m_argv;
	return *this;
}

const char* CommandLineParser::opt(const char* flag)
{
	auto e = m_argv + m_argc;
	auto itr = std::find(m_argv, m_argv + m_argc, flag);
	if (itr != e && itr++ != e)
	{
		return *itr;
	}
	return nullptr;
}

// CommandLineParser commandLineOptions;

void parse(int argc, const char* argv[])
{
	commandLineOptions = CommandLineParser(argc, argv);
}

#ifndef PALACE_GRAPHICS_VULKANCONFIG_HPP
#define PALACE_GRAPHICS_VULKANCONFIG_HPP

#include <algorithm>

class CommandLineParser
{
public:
	const int& PROP;

	CommandLineParser();
	CommandLineParser(int argc, const char* argv[]);
	CommandLineParser& operator=(CommandLineParser rhs);

private:
	int			 m_argc{};
	const char** m_argv{};
	int			 m_prop{};

	const char* opt(const char* opt);
};

extern CommandLineParser commandLineOptions;
extern void				 parse(int argc, const char* argv[]);

#endif

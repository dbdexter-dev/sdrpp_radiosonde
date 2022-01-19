#include "utils.hpp"

std::string 
getTempFile(std::string file)
{
	const char *env = getenv("TMP");    /* Unix / Apple */

	if (!env)
	   env = getenv("TEMP");            /* Windows */
	if (!env)
	   env = "/tmp";
#ifdef _WIN32
	return (std::string(env) + "/" + file);
#else
	return (std::string(env) + "\\" + file);
#endif
}

#ifndef OUTDEBUG_H_
#define OUTDEBUG_H_

#include "debug.h"

class OutDebug : public Tools::Debug
{
public:
	OutDebug();

	virtual void add( const char *file, unsigned line, const char *function, const std::string & s );
};

#endif /* OUTDEBUG_H_ */

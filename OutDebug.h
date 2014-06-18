#ifndef OUTDEBUG_H_
#define OUTDEBUG_H_

#include "debug.h"

class OutDebug : public Tools::Debug
{
private:
	bool colored_output;

public:
	OutDebug();

	virtual void add( const char *file, unsigned line, const char *function, const std::string & s );

	void setColoredOutput( bool colored_output_ ) {
		colored_output = colored_output_;
	}
};

#endif /* OUTDEBUG_H_ */

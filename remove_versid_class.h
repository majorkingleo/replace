#ifndef REMOVE_VERSID_CLASS_H
#define REMOVE_VERSID_CLASS_H

#include "HandleFile.h"

class RemoveVersid : public HandleFile
{
public:
	RemoveVersid();

	virtual std::string remove_versid( const std::string & file ) = 0;
};

#endif

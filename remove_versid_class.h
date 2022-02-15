#ifndef REMOVE_VERSID_CLASS_H
#define REMOVE_VERSID_CLASS_H

#include "HandleFile.h"

class RemoveVersid : public HandleFile
{
public:
	RemoveVersid();

	virtual std::wstring remove_versid( const std::wstring & file ) { return std::wstring(); }
};

#endif

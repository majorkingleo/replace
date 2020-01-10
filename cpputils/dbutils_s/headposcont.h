#ifndef _wamas_framework_HEADPOSCONT_H
#define _wamas_framework_HEADPOSCONT_H
/**
 * @brief Store head and position data
 * @file
 * @author Copyright (c) 2008 Salomon Automation GmbH
 */

#include <logtool2.h>
#include <if_opmsg_s.h>
#include <string>
#include <vector>
#include <convError.h>

namespace wamas {
namespace wms {

/**
 * @class HeadPosCont
 * storage for Head Positon data
 */
template <typename HEAD, typename PP>
class HeadPosCont {
public:
	
	HeadPosCont(): bIsSet_(false){
		memset(&tHead_, 0, sizeof(tHead_));
	}
	HeadPosCont(const HEAD & tHead): 
			tHead_(tHead), bIsSet_(true){}
	~HeadPosCont(){}

	int addPosition(const std::string & strFac, const PP & tPos)
	{
		if (!bIsSet_) {
			LoggingSIf_LogPrintf (strFac.c_str(), LOGGING_SIF_ALERT,
					"HeadPosCont::addPosition: Error: No Head defined");
			int iRv = OpmsgSIf_ErrPush(GeneralTerrInternal, NULL);
			throw ::convert(iRv);
		}
		vecPos_.push_back(tPos);
		return 1;
	}
	bool isSet() const { return bIsSet_; }
	const HEAD& getHead() const { return tHead_; }
	const std::vector<PP>& getPosArr() const { return vecPos_; }

private:
	
	HEAD					tHead_;
	std::vector<PP>			vecPos_;
	bool					bIsSet_;
};

} // /namespace wms
} // /namespace wamas

#endif //_wamas_framework_HEADPOSCONT_H


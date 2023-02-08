/**
 * @file lo2str_enum_util.cc
 * @todo LO2STR enum utility functions
 * @author mmattl 
 * Copyright (c) 2022 SSI Schaefer IT Solutions
 */
#include <cpp_util.h>
#include <ml.h>
#include <ml_util.h>

#include "lo2str_enum_util.h"

using namespace std;
using namespace Tools;

string Tools::enumValToStr (const LO2STR &lo2StrDef, long lSelVal, const string &sepSign) {

    string selEnumValStr;

    for (size_t currVal = 0; currVal < lo2StrDef.l2sSize; currVal++) {
        if (((1<<currVal) & lSelVal) != 0) {
            if (selEnumValStr.empty()) {
                selEnumValStr = MlMsg(l2sGetNameByValue (const_cast<LO2STR *>(&lo2StrDef), (long)currVal));
            } else {
                selEnumValStr += format("%s%s",
                    sepSign, 
                    MlMsg(l2sGetNameByValue (const_cast<LO2STR *>(&lo2StrDef), (long)currVal)));
            }
        }
    }

    return selEnumValStr;
}

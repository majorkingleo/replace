#ifndef LO2STR_ENUM_UTIL
#define LO2STR_ENUM_UTIL

#ifndef NOWAMAS
#include <lo2str.h>


namespace Tools {

/**
 * Build string with labels of selected enum values
 * e.g. BEREIT,AKTIV,VERLB,FERTIG 
 * 
 * PARAM:
 *      lo2StrDef ...   the LO2STR definition of enum, PDL generated, e.g. l2s_AUSKSTATUS
 *      lSelVal   ...   a long holding the bitmask of selected enum values
 *      sepSign   ...   optional separator sign - defaults to ",""
 * 
 * RETURNS:
 *      std::string ... with the text (label) of selected enum values
 * 
 * THROWS:
 *      nothing
 */
std::string enumValToStr (const LO2STR &lo2StrDef, long lSelVal, const std::string &sepSign = ",");

} // namespace Tools

#endif // NOWAMAS
#endif // LO2STR_ENUM_UTIL

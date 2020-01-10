/**
 * @file
 * @todo describe file content
 * @author Copyright (c) 2009 Salomon Automation GmbH
 */

#include <cpp_util.h>
#include <wamasbox.h>

#include "listbuffer_utils.h"


namespace Tools {

void ListBufferDoSelection( ListBuffer lb, int hint )
{
  if( lb == NULL )
	throw REPORT_EXCEPTION( "Listbuffer pointer ist null" );

  long last=0;
  long l;
  ListElement le;

  last    = ListBufferNumberOfElements(lb,LIST_HINT_ALL)-1;
  for (l=0;l<=last;l++) {
	le  = ListBufferGetElement(lb,l);
	if (le) {
	  le->hint    = hint;
	}
  }
  ListBufferUpdate(lb);
}

ListElement find_lb_sel(ListBuffer lb, long start, long *found)
{
    ListElement le;
    long        l;
    long        last;
    ListElement rv  = NULL;

    if (lb && start>=0) {
        last    = ListBufferNumberOfElements(lb,LIST_HINT_ALL)-1;
        for (l=start;l<=last;l++) {
            le  = ListBufferGetElement(lb,l);
            if (le) {
                if (OWBIT_GET(le->hint,LIST_HINT_SELECTED)) {
                    rv  = le;
                    if (found) {
                        *found  = l;
                    }
                    break;
                }
            }
        }
    }
    return rv;
}

bool ListBufferCheckIfAnySelected( ListBuffer buf, MskDialog mask )
{
  if( buf == NULL )
	throw REPORT_EXCEPTION( "Listbuffer pointer ist null" );
  
  long l = 0;
  
  if( find_lb_sel(buf,l,&l) != NULL )
	return true;

  WamasBoxAlert (SHELL_OF(mask),
				 WboxNtext, MlM("Bitte wählen Sie einen Eintrag aus."),				  
				 NULL );

   return false;
}

} // /namespace wamas


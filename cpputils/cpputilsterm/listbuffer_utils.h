/**
 * @file
 * @todo describe file content
 * @author Copyright (c) 2009 Salomon Automation GmbH
 */

#ifndef _Tools_LISTBUFFER_UTILS_H
#define _Tools_LISTBUFFER_UTILS_H

#include <owil.h>
#include <vector>
#include <cpp_util.h>

namespace Tools {

  ListElement find_lb_sel(ListBuffer lb, long start, long *found);
  
  void ListBufferDoSelection( ListBuffer lb, int hint );

  inline void ListBufferSelectAll( ListBuffer lb ) {
	ListBufferDoSelection( lb, LIST_HINT_SELECTED );
  }

  inline void ListBufferSelectNone( ListBuffer lb ) {
	ListBufferDoSelection( lb, LIST_HINT_NONE );
  }

  template<class T> T* ListBufferGetSelectedEntry( ListBuffer buf )
	{
	  if( buf == NULL )
		throw REPORT_EXCEPTION( "Listbuffer pointer ist null" );

	  long l = 0;
	  ListElement	le=NULL;
	  
	  do
		{
		  le  = Tools::find_lb_sel(buf,l,&l);
		  if (le)
			{
			  l++;
			  T *lbe;
			  lbe = (T*)le->data;
			  if( lbe )
				return lbe;
			}
		} while( false );
	  
	  return NULL;
	}

  // Copies the selected  ListBuffer entries into a vector
  template<class T> std::vector<T> ListBufferGetCopyOfSelectedEntries( ListBuffer buf )
		{
	  if( buf == NULL )
	  		throw REPORT_EXCEPTION( "Listbuffer pointer ist null" );

	  	  long l = 0;
	  	  ListElement	le=NULL;
	  	  std::vector<T> entries;

	  	  do
	  		{
	  		  le  = Tools::find_lb_sel(buf,l,&l);
	  		  if (le)
	  			{
	  			  l++;
	  			  T *lbe;
	  			  lbe = (T*)le->data;
	  			  if( lbe ) {
	  				entries.push_back(*lbe);
	  			  }
	  			}
	  		  else
	  			{
	  			  break;
	  			}
	  		} while( true );

	  	  return entries;
		}

  // Makes pointers from selected ListBuffer entries and copies these pointers into a vector
  template<class T> std::vector<T*> ListBufferGetSelectedEntries( ListBuffer buf )
	{
	  if( buf == NULL )
		throw REPORT_EXCEPTION( "Listbuffer pointer ist null" );

	  long l = 0;
	  ListElement	le=NULL;
	  std::vector<T*> entries;

	  do
		{
		  le  = Tools::find_lb_sel(buf,l,&l);
		  if (le)
			{
			  l++;
			  T *lbe;
			  lbe = (T*)le->data;
			  if( lbe ) {
				entries.push_back(lbe);
			  }
			}
		  else
			{
			  break;
			}
		} while( true );
	  
	  return entries;
	}
  // Copies all ListBuffer entries into a vector
  template<class T> std::vector<T> ListBufferGetCopyOfAllEntries( ListBuffer buf )
  	{
  	  if( buf == NULL )
  		throw REPORT_EXCEPTION( "Listbuffer pointer ist null" );

  	  long l = 0;
  	  ListElement	le=NULL;
  	  std::vector<T> entries;

  	  long last = ListBufferNumberOfElements(buf,LIST_HINT_ALL)-1;
  	  for( l = 0; l <= last; l++ )
  		{
  		  le = ListBufferGetElement(buf,l);
  		  T *lbe;
  		  lbe = (T*)le->data;
  		  if( lbe ) {
  			entries.push_back(*lbe);
  		  }
  		}

  	  return entries;
  	}

  // Makes pointers from all ListBuffer entries and copies these pointers into a vector
  template<class T> std::vector<T*> ListBufferGetAllEntries( ListBuffer buf )
	{
	  if( buf == NULL )
		throw REPORT_EXCEPTION( "Listbuffer pointer ist null" );

	  long l = 0;
	  ListElement	le=NULL;
	  std::vector<T*> entries;

	  long last = ListBufferNumberOfElements(buf,LIST_HINT_ALL)-1;
	  for( l = 0; l <= last; l++ )
		{
		  le = ListBufferGetElement(buf,l);
		  T *lbe;
		  lbe = (T*)le->data;
		  if( lbe ) {
			entries.push_back(lbe);
		  }
		}
	  
	  return entries;
	}

  bool ListBufferCheckIfAnySelected( ListBuffer buf, MskDialog mask );

  inline bool ListBufferCheckIfAnyIsSelected( ListBuffer buf, MskDialog mask )
	{
	  return ListBufferCheckIfAnySelected( buf, mask );
	}

  /**
   * With ListBufferdo you can do an operation on any element on the listbuffer,
   * by defining a functor class.
   *
   * Eg:
   *
   * class ColorByTep
   * {
   *    long lPosNr;
   *	Color farbe;
   *
   *  public:
   *  ColorByTep( const TEP & tep, Color farbe_ )
   *	: lPosNr( tep.tepPosNr ),
   *	  farbe( farbe_ )
   *	  {}
   *
   *	void operator()( ListBuffer buf, ListElement le, long l, MezulagLbfRecTeArt* pTeArt) const
   *	{
   *	  if( !pTeArt ) {
   *		return;
   *      }
   *	  
   *	  if( pTeArt->tep.tepPosNr == lPosNr ) {
   *		  ListBufferSetElementColor ( buf, l, farbe );
   *		}
   *	}
   *  };
   *
   *  Usage:
   *
   *  Color farbe  = GrColorLock (C_LIGHTGRAY);
   *  ListBufferDo<MezulagLbfRecTeArt>( eArtStore_.GetTeArt(), ColorByTep( quellTep, farbe ) );
   *  GrColorUnlock (farbe);
   */
  template<class T, class Operator> void ListBufferDo( ListBuffer buf, const Operator & op )
	{
	  if( buf == NULL )
		throw REPORT_EXCEPTION( "Listbuffer pointer ist null" );
	  
	  long l = 0;
	  ListElement	le=NULL;
	  
	  long last = ListBufferNumberOfElements(buf,LIST_HINT_ALL)-1;
	  for( l = 0; l <= last; l++ )
		{
		  le = ListBufferGetElement(buf,l);
		  T *lbe;
		  lbe = (T*)le->data;
		  if( lbe ) {
			op( buf, le, l, lbe );
		  }
		} 
	}

} // /namespace Tools

#endif  /* _wamas_TERM_INVUTILS_H */

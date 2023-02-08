#define MSK_INTERN
#include "cpplist_util.h"
#include <owrcloader.h>
#include <string_utils.h>
#include <format.h>
#include <sqltable.h>
#include <t_util.h>
#include <wamasbox.h>
#include <listgen.h>

#include <logtool2.h>
#include "efcbm.h"
#include "efcb_std.h"
#include <iterator.h>

using namespace Tools;

int StandardList::list_lines = 22;
bool StandardList::defaultListenToAllAsciiKeys = false;
bool StandardList::start_modal_by_default = false;
StandardList::PrimanListSelCallbackType StandardList::DefaultPrimanListSelCallback = NULL;
StandardList::HOOK_LIST StandardList::standard_list_hooks;

void StandardList::set_list_lines( int lines )
{
	list_lines = lines;
}

void StandardList::setDefaultListenToAllAsciiKeys( bool value )
{
  defaultListenToAllAsciiKeys = value;
}

extern "C" void destroy_callback_default (ListTdialog *ptListDialog)
{
	if (ptListDialog)
		free(ptListDialog);
	return;
}

StandardList::StandardList()
  : rc_files(),
    title(),
    id_name(),
    sel_mask(),
    table_name(),
    part_name(),
    part_table_name(),
    atom_name(),
    wart_dlg_name(),
    auto_free_part_record( true ),
    part_names(),
    user_data(0),
    plistdialog(0),
    p_selector(0),
    selector_size(0),
    sql_stmt_printed(false),
    keyCode(0),
    keyCodeUpper(0),
    listenToAllAsciiKeys(defaultListenToAllAsciiKeys),
    start_blocking(false),
    start_direct(false),
    hEh(0),
    lRecNum(0),
    enable_all_filter_fields(false),
    fac(FAC_CPPLIST_UTIL),
	filter_mask_rl(0),
	filter_titel_bis_ausblenden(false)
{
}

StandardList::StandardList( const StandardList & list )
: rc_files(),
  title(),
  id_name(),
  sel_mask(),
  table_name(),
  part_name(),
  part_table_name(),
  atom_name(),
  wart_dlg_name(),
  auto_free_part_record( true ),
  part_names(),
  user_data(0),
  plistdialog(0),
  p_selector(0),
  selector_size(0),
  sql_stmt_printed(false),
  keyCode(0),
  keyCodeUpper(0),
  listenToAllAsciiKeys(defaultListenToAllAsciiKeys),
  start_blocking(false),
  start_direct(false),
  hEh(0),
  lRecNum(0),
  enable_all_filter_fields(false),
  fac(),
  filter_mask_rl(0),
  filter_titel_bis_ausblenden(false)
{
}

StandardList & StandardList::operator=( const StandardList & list )
{
  return *this;
}

void StandardList::my_destroy_callback( ListTdialog *ptListDialog )
{
  StandardList *sl = (StandardList*) ptListDialog->ldUserData;
  
  if( sl )
	sl->destroy_my_self( ptListDialog );
}

int StandardList::my_sel_callback( elThandle *hEh, int iSel_reason, OWidget hW, 
								selTdata *ptSelData, void *pvCalldat )
{
  StandardList *sl = (StandardList*) pvCalldat;

  if( sl )
	return sl->selection_callback( hEh, iSel_reason, hW, ptSelData );
  
  return 0;
}

#if TOOLS_VERSION < 37
int StandardList::my_cb_sqlList(lgRecords *sql, ListTdialog *list, int reason, char *stmt, void *ud)
#else
int StandardList::my_cb_sqlList(lgRecords *sql, ListTdialog *list, int reason, const char *stmt, void *ud)
#endif
{
  if( list )
	{
	  StandardList *sl = (StandardList*) list->ldUserData;	  

	  if( sl )
		return sl->sql_callback( sql, list, reason, stmt, ud );
	}

  return 0;
}

/* sis: Generator Callback */
int StandardList::my_gen_callback(void *cb, int reason)
{
   ListTXdialog            *li=(ListTXdialog *)cb;
   ListTdialog 		   *list=li->list;
 
   if( list )
   {
	  StandardList *sl = (StandardList*) list->ldUserData;
	  
	  if( sl )
		return sl->gen_callback( cb, reason );
	}
  return 0;
}

int StandardList::handleKeyCode( CbView  cbv )
{
  char key = ' ';

  if (cbv->u.input.xreason==VI_MOUSE || cbv->u.input.xreason==VI_TRAVERSE) {
    MousePressed();
  } else if (cbv->u.input.xreason==VI_KEY) {
    key = (char)((CharEvent *)cbv->u.input.sysinfo)->key;

    setKeyCode(key);
    setKeyCodeUpper(toupper(key));

    std::cout << "key pressed: " << key << std::endl;
  }

#if TOOLS_VERSION >= 38
  return SEL_REASON_USER;
#else
  return 0;
#endif
}

/** Zeigt die Anzahl der Zeilen in der Liste beim Drücken der Taste
 * 'I' an. Sofern dies mit setListenToAllAsciiKey() aktiviert wurde.
 * Ursprung bei Festo
 */
bool StandardList::displayLineInfo( elThandle *hEh, OWidget hW )
{
  if( hEh != NULL  &&
      hEh->ehLM2_DESC != NULL )
    {

      if( getKeyCodeToUpper() == 'I' ) {
          LM2_INFO *info = 0;

          if ((info = lm2_get_info(hEh->ehLM2_DESC)))
            {
              WamasBox(hW,
                  WboxNboxType,WBOX_INFO,
                  WboxNbutton,    WboxbNok,
                  WboxNmwmTitle,  MlM("Listen Info"),
                  WboxNtext,      TO_CHAR( format("Anzahl Zeilen %d", info->li_no_lins) ),
                  NULL );
              return false;
            }
      }
    }

  return true;
}

int StandardList::selection_callback( elThandle *hEh_, int iSel_reason, OWidget hW,
									   selTdata *ptSelData )
{
  LM2_DESC	*ptLd;
  CbViewRec	*ptCbv;
  long		lLine,lRecoffset, lType, lLiType;
  char		*tRecord;
  unsigned	iObjSize;

#if TOOLS_VERSION < 38
   /* Das funktioniert auch vor tools 37 nur deswegen weil ich 
	* auf meinem Rechner die tools gehackt habe.
	* Also davon bitte nicht irritieren lassen.
	* Das mit den Keycodes geht erst ab TOOLS 38
	**/
  if( (&ptSelData->selView) != NULL ) {
      handleKeyCode(&ptSelData->selView);
  }
#endif  

  hEh = hEh_;

  if( !displayLineInfo( hEh, hW ) )
    return 0;

  switch (iSel_reason)
	{
#if TOOLS_VERSION >= 38	
	case SEL_REASON_USER:	
#endif	
	case SEL_REASON_SEL:
	  {
	    int rv = SEL_RETURN_NOACT;
//	  if( wart_dlg_name.empty() && part_names.empty() )
//	  	break;

	  iObjSize = getPartStructSize();
	  if (iObjSize < sizeof (ListTfilter))
		iObjSize = sizeof (ListTfilter);
	  
	  ptLd = hEh->ehLM2_DESC;
	  ptCbv = &ptSelData->selView;
	  lLine = ptCbv->u.input.line;
	  
	  if (lm2_lin2rec(ptLd, lLine, &lRecNum, &lRecoffset) < 0)
		return 0;
	  if (lm2_seek(ptLd, lRecNum, 0) < 0)
		return 0;

	  tRecord = (char*)malloc (iObjSize);
	  if (tRecord == NULL)
		break;
	  memset (tRecord, 0, iObjSize);

	  std::cout << format( "record address: 0x%X\n", (void*)tRecord );

	  if (lm2_read(ptLd, &lType, tRecord) < 0) {
		free (tRecord);
		return 0;
	  }

	  // create a default entry
	  if( part_names.empty() )
	    {
	      Part part( part_name, part_table_name, wart_dlg_name, atom_name );

	      if( part.table.empty() )
		{
		  std::cout << format( "part_table_name empty defaulting to table_name (%s)\n",
				       table_name );
		  part.table = table_name;
		}

	      part_names.push_back( part );
	    }

	  for( unsigned i = 0; i < part_names.size(); i++ )
	    {
	      lLiType = elPart2LM2_TYPE(hEh->ehList, elNbody, TO_CHAR( part_names[i].name ) );

	      printf( "lType: %ld == %ld\n", lType , lLiType ); 

	      if (lType == lLiType) 
		{
		  // entry_fes(GetRootShell (), &tRecord[0]);
		  // std::cout << format( "start wart_menu: %s\n", part_names[i].wart_dlg_name );

	          // modales Starten

	          OWidget hShell = (OWidget)WdgFindShell(hW);

                  bool regen_view = do_start_wart_menu( hShell, &tRecord[0],
                      part_names[i].table,
                      part_names[i].wart_dlg_name,
                      part_names[i].atom_name );


	          /*
		  bool regen_view = do_start_wart_menu( GetRootShell(), &tRecord[0],
		      part_names[i].table,
		      part_names[i].wart_dlg_name,
		      part_names[i].atom_name );
                  */

		  if( regen_view ) {
		      rv = SEL_RETURN_REGEN_VIEW;
		  }
		  break;
		}
	    }

	  if( auto_free_part_record )
	    {
	      std::cout << format( "freeing Record: 0x%X\n", (void*)tRecord );
	      free (tRecord);
	    }

	  return rv;
	  }

	case SEL_REASON_PROCESS:
		return 0;

	case SEL_REASON_END:
		return 0;

	case SEL_REASON_PRINT:
		return doPrint( hEh , iSel_reason, hW, ptSelData );

	default:
		return 0;
	}
	return 0;
}

void StandardList::destroy_my_self( ListTdialog *ptListDialog )
{
  free( ptListDialog );
  delete this;
}

int StandardList::startDirect( OWidget parent, Value data, MskDialog *mask_rl, bool blocking )
{
  start_direct = true;
  start_blocking = blocking;
  return start( parent, data, mask_rl );
}

int StandardList::my_list_view_intput_reason_filter(CbView cbv, DSP_LIST* dsp)
{
	StandardList *list = (StandardList*)dsp->lst->ldUserData;
	return list->listViewIntputReasonFilter(cbv, dsp );
}

int StandardList::listViewIntputReasonFilter(CbView cbv, DSP_LIST* dsp)
{
	int rv = handleKeyCode( cbv );

	return rv;	
}


// http://wiki.salomon.at/wiki/index.php/Listen#Selbstdefinierte_List_Items
// behebt dynamisch den Fehler im Selektor
void StandardList::fixSelector( listSelItem     *pSelector ) const
{
  for( listSelItem *pSel = pSelector; pSel != NULL; pSel++ )
    {
      if( pSel->lsiLabel == NULL )
        break;

      elTlist *list = pSel->lsiSort.lsList;

      if( list == NULL )
        continue;

      for( elTlistPart *elPartList = list->elPartList;  elPartList != NULL; elPartList++ )
        {
          if( elPartList->elpName == NULL )
            break;

          for(  elTitem     * elpItemList = elPartList->elpItemList; elpItemList != NULL ; elpItemList++ )
            {
              if( elpItemList->elItemName == NULL )
                break;

              elTatom    & elHeader = elpItemList->elHeader;

              if( elHeader.elaFmt != NULL )
                {
                  LogPrintf( fac, LT_DEBUG, "verwerfe Formatierung im Header: %s für Element: %s",
                      elHeader.elaFmt, elpItemList->elItemName );


                  elHeader.elaFmt = NULL;
                }
            }
        }
    }
}



int StandardList::SetFocusToFirstElement_int(Value udata, WdgTtimer timer)
{
  StandardList *list = (StandardList*)udata;
  return list->SetFocusToFirstElement( udata, timer );
}

int StandardList::MskChangeFocus(MskDialog mask_rl, OWidget w)
{
    int rv;

    rv      = 0;

#if OWIL_VERSOIN >= 64 // MOBERZA WdgNisRealized ist vorher unbekannt. TODO Implementation der Funktion fuer alter Owil Versionen.
    if ((w != NULL) && (WdgIsTraversable(w))) {
        if (WdgGet(w, WdgNisRealized) && (w == (OWidget)WdgGuiGet(GuiNfocusWidget))) {
            WdgKillFocusEvent();
        }
        WdgProcessTraversal(w,TRAVERSE_CURRENT);
        rv  = 1;
    }
#endif

    return rv;
}



int StandardList::SetFocusToFirstElement(Value udata, WdgTtimer timer)
{
  // Focus auf das erste Element setzten
  for (MskTmaskDescrList *em=filter_mask_rl->em;em != NULL;em = em->next)
    {
      MskTgenericRl *ef_rl   = MskQueryRl(filter_mask_rl,em->m_descr->ef_descr,em->m_descr->key);

      if( ef_rl == NULL ) {
          continue;
      }

      // MskElementSet(ef_rl, MskNtextFontName, (Value)pacFont);

      OWidget hOw = ef_rl->ec_rl.w;
      char *vWdgClass = (char *)WdgGet(hOw, WdgNclassname);
      if( vWdgClass != NULL )
        // std::cout << "vWdgClass: " << vWdgClass << std::endl;

        // das erste Editfeld bekommt den Focus
        if( strcmp(vWdgClass, WdgClassSedit) == 0 )
          {
           MskChangeFocus( filter_mask_rl, hOw );
           break;
          }
    }

  return 0;
}


int StandardList::FilterMaskAfterCM_int(Value udata, WdgTtimer timer)
{
  StandardList *list = (StandardList*)udata;
  return list->FilterMaskAfterCM( udata, timer );
}

int StandardList::FilterMaskAfterCM(Value udata, WdgTtimer timer)
{
  for( Tools::Iterator<HOOK_LIST::iterator> it = standard_list_hooks.begin(); it != standard_list_hooks.end(); it++ )
  {
	  it->FilterMaskAfterCM( this );
  }

  return 0;
}

int StandardList::start( OWidget parent, Value data, MskDialog *pmask_rl )
{
  user_data = data;

  std::vector<std::string> sl = split_and_strip_simple( rc_files, "," );
  
  for( unsigned i = 0; i < sl.size(); i++ )
	{
	  int ret = OwrcLoadObject(ApNconfigFile,const_cast<char*>( sl[i].c_str() ) );

	  if( ret != 1 ) {
		std::cerr << "cannot load '" <<  sl[i] << "'" << std::endl;
	  }
	}


  ListTdialog		*pListDialog;
	ListTXdialog	*pListXDialog;
	ListTaction		*pListAction;
	listSelItem		*pSelector;
	MskDialog		mask_rl = 0;

#if TOOLS_VERSION < 40
 #define AS_MUMALLOC_RESOURCE(x) (void**)x
#else
 #define AS_MUMALLOC_RESOURCE(x) x
#endif // TOOLS_VERSION

	if ( VaMultipleMalloc (	
        sizeof(ListTdialog),  AS_MUMALLOC_RESOURCE(&pListDialog),
				sizeof(ListTXdialog), &pListXDialog,
				sizeof(ListTaction),  &pListAction,
				getSelectorSize(),	  &pSelector,
				(ssize_t)-1 ) == (void*)NULL )
	  {
		  return -1;
	  }

	// fillSelector( p_selector );
	plistdialog = pListDialog;

	memset(pListDialog, 0, sizeof(ListTdialog));
	pListDialog ->ldSelector			= pSelector;
	pListDialog->ldHsl				= HSL_NI;
	pListDialog->ldAction			= pListAction;
/*
	pListDialog->ldGenCallback		= lgGenSqlList;
*/
	pListDialog->ldGenCallback		= my_gen_callback;
	pListDialog->ldTitle			= TO_CHAR( title );
	pListDialog->ldGenCalldata		= pListXDialog;
	pListDialog->ldDestroyCallback	= my_destroy_callback;
	pListDialog->ldPMaskRl			= pmask_rl;
	pListDialog->ldSelMask			= sel_mask.empty() ? NULL : TO_CHAR( sel_mask );
	pListDialog->ldUserData         = this;
	pListDialog->ldSelectPrinter    = getPrinterFunc();
	pListDialog->ldPrintButResValTable = mgPrnButResValTable;


	memcpy(pSelector, getSelector(), getSelectorSize());
	
	fixSelector(pSelector);

	memset(pListXDialog, 0, sizeof(ListTXdialog));
	pListXDialog->list				= pListDialog;
	pListXDialog->cb_genSqlList		= my_cb_sqlList;
	pListXDialog->bd 				= NULL;
	pListXDialog->listLines 	    = list_lines;

	memset(pListAction, 0, sizeof(ListTaction));
	
	pListAction->sel_callback		= my_sel_callback;
	pListAction->sel_calldata       = this;

	pListAction->sel_nookbutton     = 1;

	if( listenToAllAsciiKeys ) {
#if TOOLS_VERSION >= 38		
	    // siehe Bugzilla http://bugzilla.salomon.at/show_bug.cgi?id=31435
		ListDialogSet(pListDialog, ListNviewCbInputReasonFilter, CAST_TO_CALLBACK_VALUE_TYPE(my_list_view_intput_reason_filter));
#endif		
	}
	// elSetStdListWidth(100);
	// ListDialogSet(pListDialog, ListNviewWidth, (void*)200);

    /* MOBERZA Wird als MaskId NULL uebergeben so generieren sich die
     * Tools den Maskennamen selber: LITMASK_1406707344 die Nummer ist
     * einfach der aktuelle Zeitstempel in Sekunden.
     * Wenn nun aber zwei verschiedene Listen innerhalb einer Sekunde
     * geoeffnet werden, haben 2 grundverschiedene Masken den selben 
     * Namen. Dadurch stuerzt dann beim Schliessen das Programm ab.
     * Das ganz kommt vor wenn im Dialog 437 (Liste AET) dort ein
     * Lager ausgewahlt wird und dann in der Liste 2mal schnell
     * Enter gedrueckt wird. => li_aet => li_adok.
     * Das ganze betrifft naturlich nur statische Listen.
     */

    char *pcMaskId = NULL;
    std::string mask_id;

    if( sel_mask.empty() ) {
        mask_id = format("CPPLIMASK_%p", pListDialog);
        pcMaskId = TO_CHAR(mask_id);
    }

	if( start_direct )
	  {
	    mask_rl = listSelMaskOpen (pListDialog, pcMaskId);
	    if (mask_rl == (MskDialog )NULL) {
	        if (pmask_rl) {
	            *pmask_rl = (MskDialog )NULL;
	        }
	        free (pListDialog);
	        return 0 ;
	    }

	    std::cout << "mask_rl direkt " << mask_rl << std::endl;

	    pListXDialog->mask_rl = mask_rl;

            fillQueryDirect( pListDialog->ldQuery );

            if( start_blocking )
              ListDialogSet(pListDialog, ListNisModalMask, CAST_TO_CALLBACK_VALUE_TYPE(1));

            int ret = ldDoList(parent,pListDialog, start_blocking );

            listSelMaskClose (pListDialog);

            return ret;
	  }
	else
	  {
	    if (listCreateSelDup(pListDialog) < 0) {
	        free(pListDialog);
	        return 0;
	    }

	    fillSelector( pListDialog->ldSelDup );

	    filter_mask_rl = listSelMaskOpen (pListDialog, pcMaskId);
	    if (filter_mask_rl == (MskDialog )NULL) {
	        if (pmask_rl) {
	            *pmask_rl = (MskDialog )NULL;
	        }
	        free (pListDialog);
	        return 0 ;
	    }

	    std::cout << "mask_rl normal " << filter_mask_rl << std::endl;

	    pListXDialog->mask_rl = filter_mask_rl;


	    std::string lqname = getLqMaskName();

	    if( !lqname.empty() )
	      {
	        pListDialog->ldSelDup->lsiSort.lsLQmask  |= listGetLQmask(plistdialog,(char*)lqname.c_str());
	        //pListDialog->ldQuery.liqSort.lsLQmask = listGetLQmask(pListDialog, (char *)lqname.c_str() );
	        if( pListDialog->ldQuery.liqSort.lsLQmask != 0 )
	          {
	            std::cout << "lqName " << lqname << " not found\n" << std::endl;
	          }
	      }

	    if( enable_all_filter_fields ) {
	        enableAllFilterFields();
	    }

	    if (filter_titel_bis_ausblenden) {
	    	MskVaAssign( filter_mask_rl, MskGetElement( "ListLabelBis" ),
	    			MskNkey,            (Value)KEY_DEF,
					MskNattrOr, EF_ATTR_INVISIBLE | EF_ATTR_NOLABEL,
					MskNupdate, 1,
					NULL );
	    }


	    MskUpdateMaskVar(mask_rl);

        // Nur, wenn es sich um eine statische Liste handelt.
        // Bei RC Listen funktioniert das Problemlos.
        if( sel_mask.empty() )
          {
            WdgAddTimeOut(1, SetFocusToFirstElement_int, (Value)this );
          }

        WdgAddTimeOut(1, FilterMaskAfterCM_int, (Value)this );

	    // Beim Festo immer Modal starten
	    if( start_modal_by_default )
	      ListDialogSet(pListDialog, ListNisModalMask, CAST_TO_CALLBACK_VALUE_TYPE(1));

	    return listSelMask(pListDialog, parent);
	  }
}

void StandardList::enableAllFilterFields()
{
  // Das bewirkt, dass alle Filter eingeblendet sind. Bigzilla: 32247
  listSelItem     *selct;
  size_t          unSelIdx;
  for (unSelIdx = 0, selct = plistdialog->ldSelDup;
      unSelIdx < plistdialog->ldSelSize; ++unSelIdx, ++selct) {

      selct->lsiSort.lsLQmask = -1L;
  }
}

void StandardList::fillQueryDirect( liQueryDesc & ldQuery )
{
  std::cout << "fillQueryDirect called, but not implemented\n";
}

void StandardList::fillSelector( listSelItem *selector )
{
  std::cout << "fillSelector not implemented\n";
}


std::string StandardList::getLqMaskName()
{
  return std::string();
}

unsigned long StandardList::getSelectorSize()
{
  
  if( !p_selector )
	GetSelector( p_selector, selector_size );
  
  return selector_size;
}

listSelItem* StandardList::getSelector()
{
  if( !p_selector )
	GetSelector( p_selector, selector_size );
  
  return p_selector;
}

bool StandardList::do_start_wart_menu( OWidget w, void *record, 
				       const std::string & TableName, 
				       const std::string & DlgName, 
				       const std::string & AtomName  )
{
  return true;
}


#if TOOLS_VERSION < 37
int StandardList::sql_callback( lgRecords *sql, ListTdialog *list, int reason, char *stmt, void *ud )
#else
int StandardList::sql_callback( lgRecords *sql, ListTdialog *list, int reason, const char *stmt, void *ud )
#endif
{
  if( !sql_stmt_printed )
	{
	  std::cerr << "=> GO stmt:\n" << sql->SqlStmt << std::endl;
  	  sql_stmt_printed = true;
	}

  // Die Tools koennen nattülich keinerlei Nullpointer überprüfen
  if( reason == genListWriteFoot ) {
      if( sql->footRec != NULL )
        return cb_makeListFoot( sql, list, reason, stmt, ud );
  }

  return SQL_NO_ACT;
}

/* sis: Default Generator Callback */
int StandardList::gen_callback(void *cd, int reason)
{
  return lgGenSqlList(cd, reason);
}

/* sis:
 * Replace-Funktion
 */
void StandardList::_uwc(char *s)
{
    char *p;

    if (s&&*s) {
        do {
            p=strchr(s,'*');
            if (p) {
                *p  = '%';
                memmove(p+1,p,strlen(p)+1);
                *p  = '%';
                s=p+2;
            } else {
                p=strchr(s,'?');
                if (p)
                    *p  = '_';
            }
        } while (p);
    }
}


/* sis: Diverse Tool-Funktions */
char* StandardList::AddStrF(LQ_DESC *pLq, const char *sqlfilter, bool add_and_before )
{
	static char buffer[4096+1];

	memset(buffer,0,sizeof(buffer));

	const char *prefix = " ";

	if( add_and_before )
	  prefix = " and ";

	if (pLq->lq_val[0].str[0] &&
		pLq->lq_val[1].str[0])
	{
		/* Between */
		sprintf(buffer," %s %s between '%s' and '%s' ",
				prefix,
				sqlfilter, 
				pLq->lq_val[0].str,
				pLq->lq_val[1].str);
	} else if (pLq->lq_val[1].str[0])
	{
		sprintf(buffer," %s %s <= '%s' ",
				prefix,
				sqlfilter,
				pLq->lq_val[1].str);
	} else if (pLq->lq_val[0].str[0])
	{
		sprintf(buffer," %s %s like '%s' ",
				prefix,
				sqlfilter,
				pLq->lq_val[0].str);
	}
	/* Leer stürzt ab ... */
	if (buffer[0]=='\0')
	    strcpy(buffer," ");

	/* SQl-Wildcard's umwandeln ... */
	_uwc(buffer);

	return buffer;
}

/* sis: Diverse Tool-Funktions */
char* StandardList::AddTimeF(LQ_DESC *pLq, const char *StmtPart)
{
	static char buffer[4096+1];
	struct tm   ptm;
    struct tm   *ptmVon;
    struct tm   *ptmBis;
    time_t      timeVon;
    time_t      timeBis;
    char        pTimeVon[50];
    char        pTimeBis[50];

	memset(buffer,0,sizeof(buffer));
            timeVon = *(pLq->lq_val[0].intnum);
            timeBis = *(pLq->lq_val[1].intnum);
            if (timeVon != 0)
            {
                if (timeBis != 0)
                {
                    ptmVon = (struct tm*)localtime ( &timeVon );
                    /* sonst ptmVon = ptmBis (Adresse)... */
                    ptm = *ptmVon;
                    ptmBis = (struct tm*)localtime ( &timeBis );
                    strftime ( pTimeVon, sizeof ( pTimeVon ),
                                "%d-%m-%Y %H.%M.%S", &ptm );
                    strftime ( pTimeBis, sizeof ( pTimeBis ),
                                "%d-%m-%Y %H.%M.%S", ptmBis );

                    sprintf(buffer,
						" %s>=TO_DATE('%s','DD-MM-YYYY HH24.MI.SS') AND"
                        " %s<=TO_DATE('%s','DD-MM-YYYY HH24.MI.SS')",
                        StmtPart, pTimeVon, StmtPart, pTimeBis);
                }
                else
                {
                    ptmVon = (struct tm*)localtime ( &timeVon );
                    strftime ( pTimeVon, sizeof ( pTimeVon ),
                                "%d-%m-%Y %H.%M.%S", ptmVon );
                    sprintf(buffer,
						" %s=TO_DATE('%s','DD-MM-YYYY HH24.MI.SS')",
                                StmtPart, pTimeVon);
                }
            }
            else
            {
                if( timeBis != 0)
                {
                    ptmBis = (struct tm*)localtime ( &timeBis );
                    strftime ( pTimeBis, sizeof ( pTimeBis ),
                                "%d-%m-%Y %H.%M.%S", ptmBis );
                    sprintf(buffer," %s<=TO_DATE('%s','DD-MM-YYYY HH24.MI.SS')",
                                StmtPart, pTimeBis);
                }
            }

	/* Leer stürzt ab ... */
	if (buffer[0]=='\0')
	    strcpy(buffer," ");

	return buffer;
}


ListSelectionCallback::~ListSelectionCallback()
{

}

void StandardList::setDefaultPrimanListSelCallback( PrimanListSelCallbackType callback )
{
	DefaultPrimanListSelCallback = callback;
}	


void CppListEnableListenToAllAsciiKeyCodes()
{
  StandardList::setDefaultListenToAllAsciiKeys(true);
}


void CppListSetModalByDefault()
{
  StandardList::setStartModalByDefault(true);
}

int StandardList::doPrint( elThandle *hEh, int iSel_reason, OWidget hW, selTdata *ptSelData )
{
	if( DefaultPrimanListSelCallback != NULL )
	{
		return DefaultPrimanListSelCallback( hEh, iSel_reason, hW, ptSelData, this );
	}
	else
	{
		/* MOBERZA ITS-161966 in Kombination alte Tools 
		   und es wird noch kein Priman verwendet, dann muss hier 0 zurueckgegeben
		   werden, damit der Druckauswahldialog, der mit ldSelectPrinter gesetzt ist
		   verwendet wird. aufgetaucht bei Krings Herrath
		*/   
		return (0);
	}	
}

void StandardList::addHook( StandardListHook* hook )
{
	standard_list_hooks.push_back( hook );
}


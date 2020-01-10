#ifndef CPPLIST_UTIL_H
#define CPPLIST_UTIL_H

#include <logtool.h>
#include <sqllist.h>
#include <mumalloc.h>
#include <owil.h>
#include <li_util.h>
#include <ml.h>
#include <sqltable.h>
#include <vector>
#include <string>
//#include <me_util.h>
#include <cpp_util.h>
#include <ref.h>



// default destroy callback für listen
extern "C" void destroy_callback_default (ListTdialog *ptListDialog);

class StandardList
{
 public:
	class StandardListHook
	{
	public:
		virtual ~StandardListHook() {}
		virtual void FilterMaskAfterCM( StandardList *list ) = 0;
	};


  typedef printerCap  *(*ldSelectPrinterFunc)(OWidget, Value);
  typedef int (*PrimanListSelCallbackType)(elThandle *ptHandle, int iReason, OWidget hW,selTdata *ptSelData, void *pvCalldata);

  typedef std::vector<StandardListHook*> HOOK_LIST;

protected:
  static int list_lines;
  static PrimanListSelCallbackType DefaultPrimanListSelCallback;
  static HOOK_LIST standard_list_hooks;

  std::string rc_files;       // Kommaseparierte liste
  std::string title;          // Maskentitel
  std::string id_name;        // id name (fürs lists file)
  std::string sel_mask;       // Name des Filters im se_foo.rc file

  // die folgenden variablen sind nur dann notwendig, wenn ein 
  // wartungsmenü aufgerufen werden soll

  std::string table_name;      // Tabellenname des Wartungsmenü
  std::string part_name;       // PN_NAME auf den der Callback horcht
  std::string part_table_name; // Tabellenname des Wartungsmenü, 
                               // wenn dieser eventuell ein anderer ist, als der table_name

  std::string atom_name;       // atom name zb ATOM_MAK
  std::string wart_dlg_name;   // name des Wartungdialogs zb: WamDlgATOM

  bool auto_free_part_record;  // eventuell wenns speicherprobleme gibt würgaround

  /* Sollen mehrere Wartungsmenüs aufgerufen werden können,
   * so ist die Struktur entsprechend zu füllen und ein part_name.push_back() zu machen 
   * Die Variablen da oben, werden dann alle ignoriert.
   */
  struct Part 
  {
    std::string name;            // PN_NAME
    std::string table;           // tabellenname des Wartungsmenüs
    std::string wart_dlg_name;   // name des Wartungdialogs zb: WamDlgATOM
    std::string atom_name;       // atom name zb ATOM_MAK

    Part( const std::string &name, 
	  const std::string &table, 
	  const std::string &dlg,
	  const std::string &atom_name )
      : name(name), 
	 table(table), 
	 wart_dlg_name(dlg), 
	 atom_name(atom_name)
    {}
  };

  std::vector<Part> part_names;
#define FAC_CPPLIST_UTIL "cpplist_util"


  Value user_data;

  ListTdialog *plistdialog;

private:
  listSelItem * p_selector;
  unsigned long selector_size;

  StandardList( const StandardList & list );
  StandardList & operator=( const StandardList & list );
  
  bool sql_stmt_printed;

  char keyCode;
  char keyCodeUpper;
  bool listenToAllAsciiKeys;
  static bool defaultListenToAllAsciiKeys;
  static bool start_modal_by_default;

protected:

  bool start_blocking;

  bool start_direct;

  elThandle   *hEh;
  long        lRecNum;

  bool enable_all_filter_fields; // Blendet alle Filterfelder ein

  std::string fac;

  MskDialog filter_mask_rl;

  bool filter_titel_bis_ausblenden;

public:  
  StandardList();
  virtual ~StandardList() {}

  // Startfunktion
  virtual int start( OWidget parent, Value data, MskDialog *mask_rl );

  // Starte ohne die Selmask zu öffnen
  virtual int startDirect( OWidget parent, Value data, MskDialog *mask_rl, bool blocking = false );


  /* callback fürs Wartungsmenü
   * Wenn kein Wartungsmenü verwendet wird, braucht die Funktion nicht überladen werden 
   *
   * wird false zurückgegeben, so wird SEL_RETURN_NOACT an die Liste zurückgegeben
   * wird true zurückgegeben, so wird SEL_RETURN_REGEN_VIEW aufgerufen
   */
  virtual bool do_start_wart_menu( OWidget w, void *record, 
				   const std::string & TableName, 
				   const std::string & DlgName, 
				   const std::string & AtomName  );

  /* größe der großten Struktur fuers Wartungsmenue
   * muß eventuell überladen werden.
   */
  virtual unsigned getPartStructSize()
  {
	return getStructSize( TO_CHAR(table_name) );
  }

  virtual void GetSelector( listSelItem * & selector, unsigned long & size ) = 0;

  // dann überladen, wenn man sich zb seinen eigenen Footer bastelt
#if TOOLS_VERSION < 37
  virtual int sql_callback( lgRecords *sql, ListTdialog *list, int reason, char *stmt, void *ud );
#else
  virtual int sql_callback( lgRecords *sql, ListTdialog *list, int reason, const char *stmt, void *ud );
#endif

  // dann überladen, wenn man eingenes SQL-Statement braucht
  virtual int gen_callback(void *cd, int reason);

static char *AddStrF(LQ_DESC *pLq, const char *sqlfilter, bool add_and_before = true );
static char *AddTimeF(LQ_DESC *pLq, const char *StmtPart);
  static void _uwc(char *s);

  /*
   *  Die nächsten Funktionen brauch in der Regel nicht überladen zu werden.
   *  Also eigentlich nie.
   */

  // destroy callback
  virtual void destroy_my_self( ListTdialog *ptListDialog );

  /* selection callback
   * ruft do_start_wart_menü auf
   */
  virtual int selection_callback( elThandle *hEh, int iSel_reason, OWidget hW, 
								   selTdata *ptSelData );
  
  virtual listSelItem* getSelector();
  virtual unsigned long getSelectorSize();

  virtual ldSelectPrinterFunc getPrinterFunc() { return NULL; }

  void setUserSata( Value ud ) { user_data = ud; }
  Value getUserData() const { return user_data; }

  // hier den Selektor vorbefüllen
  virtual void fillSelector( listSelItem *selector );

  // hier die Tabellenspalten mit beistrichen getrennt zurückgeben, die
  // gleich von Anfang an vorgeblendet werden sollen.
  virtual std::string getLqMaskName();

  static void my_destroy_callback( ListTdialog *ptListDialog );
  static int my_sel_callback( elThandle *hEh, int iSel_reason, OWidget hW, 
							  selTdata *ptSelData, void *pvCalldat );

#if TOOLS_VERSION < 37
  static int my_cb_sqlList(lgRecords *, ListTdialog *, int, char *, void *ud);
#else
  static int my_cb_sqlList(lgRecords *, ListTdialog *, int, const char *, void *ud);
#endif

  static int my_gen_callback(void *cd, int reason);  

  static void set_list_lines( int lines );

  // Wird zum Drucken aufgerufen. Ab Owil 7 nur noch ueber diese Funktion
  virtual int doPrint( elThandle *hEh, int iSel_reason, OWidget hW, selTdata *ptSelData );

  // will man im do_start_wart_menu() auf alle keycodes reagieren, so
  // muss dieses verhalten erst aktiviert werden, hier
  // die Funktion um das Verhalten global für alle Listen zu aktivieren
  // innerhalb der Funktion do_start_wart_menu() kann dann mit getKeyCode()
  // getKeyCodeUpper() der entsprechende KeyCode abgefragt werden
  static void setDefaultListenToAllAsciiKeys( bool value );

  // will man im do_start_wart_menu() auf alle keycodes reagieren, so
  // muss dieses verhalten erst aktiviert werden,
  // innerhalb der Funktion do_start_wart_menu() kann dann mit getKeyCode()
  // getKeyCodeUpper() der entsprechende KeyCode abgefragt werden
  void setListenToAllAsciiKey( bool value );

  // gibt den Keycode zurück mit der der selection callback ausgelöst wurde
  // ist der Rückgabewert 'T', so wurde selectioncallback mit der Maus ausgelöst.
  char getKeyCode() const { return keyCode; }

  // gibt den Keycode zurück mit der der selection callback ausgelöst wurde
  // ist der Rückgabewert 'T', so wurde selectioncallback mit der Maus ausgelöst.
  char getKeyCodeToUpper() const { return keyCodeUpper; }

  virtual int handleKeyCode( CbView  cbv );

  void setKeyCode( char key ) { keyCode = key; }
  void setKeyCodeUpper( char key ) { keyCodeUpper = key; }

  virtual void MousePressed()
  {
    setKeyCode('T');
    setKeyCodeUpper('T');
  }

  /** Zeigt die Anzahl der Zeilen in der Liste beim Drücken der Taste
   * 'I' an. Sofern dies mit setListenToAllAsciiKey() aktiviert wurde.
   * false zurückgeben, wenn nach dem Aufruf dieser Funktion der weitere
   * Ablauf des selction callbacks abgebrochen werden soll.
   * Wenn man hier in diesem Callback eine WamasBox anzeigen will, so
   * ist dieses Verhalten wahrscheinlich so gewünscht.
   */
  virtual bool displayLineInfo( elThandle *hEh, OWidget hW );


  // hier den Filter befuellen, wenn die Liste direkt ohne
  // selmask gestartet wird
  virtual void fillQueryDirect( liQueryDesc & ldQuery );


  static void setStartModalByDefault( bool state )
  {
    start_modal_by_default = state;
  }

  virtual void enableAllFilterFields();

  static int my_list_view_intput_reason_filter(CbView cbv, DSP_LIST* dsp);

  virtual int listViewIntputReasonFilter(CbView cbv, DSP_LIST* dsp);

  static void setDefaultPrimanListSelCallback( PrimanListSelCallbackType callback );

  // http://wiki.salomon.at/wiki/index.php/Listen#Selbstdefinierte_List_Items
  // behebt dynamisch den Fehler im Selektor
  virtual void fixSelector( listSelItem *pSelector ) const;


  static int SetFocusToFirstElement_int(Value udata, WdgTtimer timer);

  virtual int SetFocusToFirstElement(Value udata, WdgTtimer timer);

  static int FilterMaskAfterCM_int(Value udata, WdgTtimer timer);

  virtual int FilterMaskAfterCM(Value udata, WdgTtimer timer);

  int MskChangeFocus(MskDialog mask_rl, OWidget w);

  static void addHook( StandardListHook* hook );

  MskDialog getFilterMask() const {
	  return filter_mask_rl;
  }
};

class ListSelectionCallback
{
 public:

  virtual ~ListSelectionCallback();

  virtual void do_start_wart_menu( OWidget w, void *record, 
				   const std::string & TableName, 
				   const std::string & DlgName, 
				   const std::string & AtomName  )
  {
    /* MOBERZA GCC 4.8.4 
       if this function is not implemented you will get an compiler error.
       But this function has to be subclasses in any way.
    */
    std::cout << "WARNING ListSelectionCallback::do_start_wart_menu() should be implemented\n";
  };
};

// Template Funktor zum Starten der Listen.
template<class List> class ListStarter
{
public:
  ListStarter()
  {}


  int operator()(OWidget parent, Value ud, char * title )
  {
    return operator()( parent, ud, std::string(title) );
  }

  int operator()(OWidget parent, Value ud, const char * title )
  {
    return operator()( parent, ud, std::string(title) );
  }

  int operator()(OWidget parent, Value ud, bool direct = false, bool blocking = false )
  {
	/* das geht nur deswegen, weil dies eben ein template ist
	 * und warum das Ganze?: damit man eine Maske nur einmal aufmachen kann
	 */
	static MskDialog	mask_rl = (MskDialog )NULL;
	
	if (mask_rl  &&  SHELL_OF(mask_rl)) {
		WdgGuiSet (GuiNactiveShell, (Value)SHELL_OF(mask_rl));
		return RETURN_ACCEPTED;
	}

	/* Das ist so ok, der destruktor wird über einen Callback aufgerufen,
	 * und die Liste zerstört sich dann selbst.
	 */
	List *list = new List();

	if( !direct )
          return list->start(parent, ud, &mask_rl);
	else
	  return list->startDirect(parent, ud, &mask_rl, blocking);
  }

  int operator()(OWidget parent, Value ud, const std::string & title, bool direct = false, bool blocking = false  )
  {
        /* das geht nur deswegen, weil dies eben ein template ist
         * und warum das Ganze?: damit man eine Maske nur einmal aufmachen kann
         */
        static MskDialog        mask_rl = (MskDialog )NULL;

        if (mask_rl  &&  SHELL_OF(mask_rl)) {
                WdgGuiSet (GuiNactiveShell, (Value)SHELL_OF(mask_rl));
                return RETURN_ACCEPTED;
        }

        /* Das ist so ok, der destruktor wird über einen Callback aufgerufen,
         * und die Liste zerstört sich dann selbst.
         */
        List *list = new List(title);

        if( !direct )
          return list->start(parent, ud, &mask_rl);
        else
          return list->startDirect(parent, ud, &mask_rl, blocking);
  }


  int operator()(OWidget parent, Value ud, Tools::Ref<ListSelectionCallback> cb, bool direct = false, bool blocking = false  )
  {
	/* das geht nur deswegen, weil dies eben ein template ist
	 * und warum das Ganze?: damit man eine Maske nur einmal aufmachen kann
	 */
	static MskDialog	mask_rl = (MskDialog )NULL;
	
	if (mask_rl  &&  SHELL_OF(mask_rl)) {
		WdgGuiSet (GuiNactiveShell, (Value)SHELL_OF(mask_rl));
		return RETURN_ACCEPTED;
	}

	/* Das ist so ok, der destruktor wird über einen Callback aufgerufen,
	 * und die Liste zerstört sich dann selbst.
	 */
	List *list = new List(cb);

	if( !direct )
	  return list->start(parent, ud, &mask_rl);
	else
	  return list->startDirect(parent, ud, &mask_rl, blocking);
  }
};

extern "C" void CbSetFocusTo( MskDialog mask, 
							  MskStatic ef,
							  MskElement el,
							  int reason,
							  void *cbc );

extern "C" void CppListEnableListenToAllAsciiKeyCodes();

extern "C" void CppListSetModalByDefault();

#endif


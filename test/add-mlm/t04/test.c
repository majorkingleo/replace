int main() {    
	char msg[256];

	sprintf(msg, "    %d %s buchen?    ",
        anzEle, anzEle==1?"Auslagerung":"Auslagerungen");

    GrBell();
    rv = WamasBox(SHELL_OF(ctxt->mask),
            WboxNboxType,   WBOX_WARN,
            WboxNbuttonText,"Ja",
            WboxNbuttonRv,  IS_Ok,
            WboxNbutton,    WboxbNcancel,
            WboxNescButton, WboxbNcancel,
            WboxNmwmTitle,  "Best�tigung",
            WboxNtext,      msg,
            NULL);
    if (rv != IS_Ok) {
        return 0;
    }

}

int main() {

           rv = WamasBox (SHELL_OF(mask),
                          WboxNboxType,   WBOX_WARN,
                          WboxNbuttonText,"Ja",
                          WboxNbuttonRv,  IS_Ok,
                          WboxNbutton,    WboxbNcancel,
                          WboxNescButton, WboxbNcancel,
                          WboxNmwmTitle,  "Datenexport",
                          WboxNtext,              "    Vorgang abbrechen?    ",
                          NULL);
}

int main() {

                 (void)WamasBox(pshell,
                       WboxNboxType,   WBOX_ALERT,
                       WboxNmwmTitle,  MlM("Interne Initialisierung"),
                       WboxNbutton,    WboxbNok,
                       WboxNtext,              StrForm(
			                    "UniBdCreate: size of field %d in parent join matches not "
                                "to that in child join. (%s: %s != %s)",
                                 ubidx + 1,
                                 ucr->ucrConnect->ucName,
                                 ucr->ucrConnect->ucParentJoinFields ),
								 0 );

}

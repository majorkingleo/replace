int main() {
                        WamasBox(SHELL_OF(ptMatspCtxt->hMaskRl),
                                 WboxNboxType,   WBOX_ALERT,
                                 WboxNbutton,    WboxbNok,
                                 WboxNmwmTitle,  MlM(acActionSub),
                                 WboxNtext,      StrForm("%s\n%s",
                                    acErr1, acErr2),
                                NULL);
}

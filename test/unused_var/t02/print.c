/**
* @file
* @todo describe file content
* @author Copyright (c) 2013 Salomon Automation GmbH
*/
#include "prnint.h"
#include <printcap.h>
#include <sutil20.h>
#include <pma.h>

#define FAC_DEFAULT FAC_PRN

/* [endfold] */

/* [[ Prn_Init ]] */

/* Prn Modul initialisieren.
 */
PUBLIC int Prn_Init(void) 
{
    int rv;
    prnRecPtr prnPtr=MOD_INIT(prnRec);

	REQUIRE_VAR;
	Var_Load(PRN_VAR_FILE,"Sprn");

    return RETURN_ACCEPTED;
}

/* [endfold] */
/* [[ Prn_Exit ]] */

PUBLIC int Prn_Exit(void) 
{
    prnRecPtr prnPtr=MOD_PTR;
    int rv;

    rv=MOD_EXIT;
    return rv;
}

/* [endfold] */


/* New function added for the conversion to ibm */
static int Var_ExistsListString(char *var_str,char *name,char *delimiter)
{
	char *def_delim = ",";
	char **x_str;
	int  i,rv=RETURN_NOTFOUND;
	long	ll;
	
	LogPrintf(FAC_VAR,LT_NOTIFY,"List '%s' wird gesucht '%s' delim. '%s' sub_compare %d",
			  var_str,name,delimiter,0);
	
	if (!var_str) return 0;
	if (!delimiter) delimiter = def_delim;
	
	if(var_str) {
		x_str = StrToStrArray(var_str,delimiter[0]);
		for(i=0;x_str[i];i++) {
			if(!strcmp(name,x_str[i])) {
				rv = RETURN_FOUND;
				LogPrintf(FAC_VAR,LT_NOTIFY,"'%s' Str gefunden",name);
				break;
			}
		}
		if(x_str) x_str = MemDeallocate(x_str);
	}
	return rv;	
}

/* Convert from Ansi Charset to Code Page 850 Charset */
char *cv_ansi2ibm(char *s,int convert)
{
	char *pp=NULL;
	unsigned char index;
	
	/*    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   A,   B,   C,   D,   E,   F */
	static unsigned char ansi_cp850[128]= {
		0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f, /* 80 */
		0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f, /* 90 */
		0xa0,0xa1,0xa2,0x9c,0xa4,0xa5,0xa6,0x15,0xa8,0xb8,0xaa,0xab,0xac,0xad,0xa9,0xaf, /* a0 */
		0xb0,0xb1,0xfd,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xac,0xab,0xf3,0xbf, /* b0 */
		0xb7,0xb5,0xb6,0xc7,0x8e,0x8f,0x92,0x80,0xe4,0x90,0xd2,0xd3,0xde,0xd6,0xd7,0xd8, /* c0 */
		0xd1,0xa5,0xe3,0xe0,0xe2,0xe5,0x99,0xd7,0x9d,0xeb,0xe9,0xea,0x9a,0xed,0xe8,0xe1, /* d0 */
		0x85,0xa0,0x83,0xc6,0x84,0x86,0x91,0x87,0x8a,0x82,0x88,0x89,0x8d,0xa1,0x8c,0x8b, /* e0 */
		0xf0,0xa4,0x95,0xa2,0x93,0xe4,0x94,0xf7,0x9b,0x97,0xa3,0x96,0x81,0xec,0xe7,0x98  /* f0 */
	};
	
	pp=s;
	/* Convert only if appcon-entry is true */
	if ((Var_GetBool("Sprn.ConvertAnsi",0)==1) &&
		(convert==RETURN_NOTFOUND)) {
/*		fprintf(stderr,"->'%s'\n",pp);*/
		for  (;*s;s++) {
			if ((*s)&0x80) {
				index=(*s)&~0x80;
				*s=ansi_cp850[index];
			}
		}
/*		fprintf(stderr,"<-'%s'\n",pp);*/
	}
	return pp;
}



/* [[ prn_ReadFile ]] */

/*
=head1 Lesen einer Datei
=head2 Signatur
   

=head2 Beschreibung
Eine Datei wird in einen dynamisch allozierten Pufferbereich gelesen.

=head2 Argumente
=over 3
=item fname
Name der Datei
=item buf
Zeiger auf Puffer, dynamisch alloziert, dieser muss vom Aufrufer
freigegeben werden.
=back

=head2 Rueckgabewert
liefert RETURN_ACCEPTED wenn alles ok (dann ist auch C<buf> gueltig),
oder RETURN_ERROR.
Fehler werden ins Library-Logfile geschrieben.
*/
PRIVATE int prn_ReadFile(char *fname,char **buf) 
{
	int fd;
	struct stat sbuf;
	int len;
	char *tmp;
	
	*buf=NULL;	
	fd=open(fname,O_RDONLY);
	if(fd<0) {
		LogPrintf(FAC_PRN,LT_ERROR,
				  "Kann Drucker-Eingabedatei '%s' nicht oeffnen", fname);
		return(RETURN_ERROR);
	}
	if(fstat(fd,&sbuf)!=0) {
		LogPrintf(FAC_PRN,LT_ERROR,
				  "Fehler beim Stat von '%s'.", fname);
		return(RETURN_ERROR);
	}
	len=sbuf.st_size;
	tmp=malloc(len+10);
	if(tmp==NULL) {
		LogPrintf(FAC_PRN,LT_ERROR,
				  "Fehler beim Malloc fuer '%s'.", fname);
		close(fd);
		return(RETURN_ERROR);
	}
	
	memset(tmp,0,len+9);
	if(read(fd,tmp,len)!=len) {
		LogPrintf(FAC_PRN,LT_ERROR,
				  "Fehler beim Lesen von '%s'.", fname);
		free(tmp);
		close(fd);
		return(RETURN_ERROR);
	}
	close(fd);
	*buf=tmp;
	return(RETURN_ACCEPTED);
	
}

/* [endfold] */
/* [[ prn_Condition ]] */

/*
=head1 Ersetzen von benannten Strings
=head2 Signatur
   

=head2 Beschreibung
ersetzt im Puffer C<buf> alle Vorkommnisse der durch C<#xxx#yyyy#>
eingegrenzten Strings durch den Inhalt von yyyy wenn xxx den Wert TRUE
hat und liefert einen neuen Puffer mit dem Ergebnis. Vom alten Puffer
wird angenommen, dass er mittels C<malloc()> allokiert wurde. Er wird
auf jeden Fall freigegeben. 

=head2 Argumente
=over 3
=item buf
Puffer mit zu substituierenden Werten, muss dynamisch alloziert sein,
da er in dieser Funktion freigegeben wird.
=item num
Anzahl der Eintraege im Feld C<sub>
=item sub
Name/Wert-Paare fuer die Substitution. Wird C<#name#> im Puffer
gefunden, so wird dieser durch den dahinter folgenden Wert ersetzt
wenn die Variable C<name> TRUE ist..
=item new
Zeiger auf Puffer. Dieser wird dynamisch alloziert und muss vom
Aufrufer freigegeben werden.
=back

=head2 Rueckgabewert
liefert RETURN_ACCEPTED wenn alles ok (dann ist auch C<new> gueltig),
oder RETURN_ERROR.
Fehler werden ins Library-Logfile geschrieben.
*/
PRIVATE int prn_Conditions(char *buf,int num,SubstRec sub[],char **new)
{
	int i,len,newlen;
	char *op,*sp,*rep;
	char *tmp;
    int lines;
	
	*new=NULL;
	
	if((buf==NULL)&&(buf[0]=='\0')) {
		return(RETURN_ACCEPTED);
	}
	
    lines=0;
	len=strlen(buf);
	
	/* we need a buffer that is large enough to hold the original and
	   all the subsitutions. Say we need about 50 bytes per
	   substitution */
	newlen=len+(50*num);
	tmp=malloc(newlen);
	if(tmp==NULL) {
		LogPrintf(FAC_PRN,LT_ERROR,
				  "Fehler beim Malloc fuer neuen Puffer");
		return(RETURN_ERROR);
	}
    memset(tmp,0,newlen);
	sp=op=buf;
	tmp[0]='\0';
	
	while(*op) {
        if(*op=='\n') {
            lines++;
        }
		if(*op=='#') {
			/* first copy up to this space */
			*op='\0';
			strcat(tmp,sp);
			
			/* now find out about this identifier */
			sp= ++op;
			while(*op && (*op!='#')) {
                if(*op=='\n') {
                    lines++;
                }
				++op;
			}
			*op='\0';
			for(i=0;i<num;i++) {
				if(strcmp(sub[i].var,sp)==0) {
					break;
				}
			}
            rep=++op;
            while(*op&&(*op!='#')) {
                if(*op=='\n') {
                    lines++;
                }
                ++op;
            }
            *op='\0';
			if(i<num) {
                /* ist die Variable TRUE? */
                char *tv=sub[i].val;
                if((tv==NULL)||(tv[0]=='\0')||(tv[0]=='N')||(tv[0]=='F')
                   ||(tv[0]=='0')||(tv[0]==' ')) {
                    /* wir vergessen alles bis zum naechsten '#' */
                } else if(((tv!=NULL)&&(tv[0]!='\0'))
                          &&((tv[0]=='Y')||(tv[0]=='T')
                             ||(tv[0]=='1')||(tv[0]=='J'))) {
                    /* wir vergessen alles bis zum naechsten '#' */
                    /* Wert true, kopieren des Restes */
                    strcat(tmp,rep);
                    if(strlen(tmp)>newlen) {
                        LogPrintf(FAC_PRN, LT_ERROR,
                                  "Neue Stringlaenge zu kurz (Zeile %d)",
                                  lines);
                        if (buf)free(buf);
                        return(RETURN_ERROR);
					}
				}
            
			} else {
				LogPrintf(FAC_PRN,LT_ERROR,
						  "Fehler beim Subst von '%s' in %d."
                          " Variable nicht vorhanden",
                          sp,lines);
				if (buf)free(buf);
				if (tmp)free(tmp);
				return(RETURN_ERROR);
			}
			sp=op+1;				
		}
		++op;
	}
	strcat(tmp,sp);	

	if (buf) free(buf);
	*new=tmp;
	return(RETURN_ACCEPTED);
}

/* [endfold] */
/* [[ prn_Substitute ]] */

/*
=head1 Ersetzen von benannten Strings
=head2 Signatur
   

=head2 Beschreibung
ersetzt im Puffer C<buf> alle Vorkommnisse der durch C<@xxx@>
eingegrenzten Strings durch die jeweiligen Variablen und liefert einen
neuen Puffer mit dem Ergebnis. Vom alten Puffer wird angenommen, dass
er mittels C<malloc()> allokiert wurde. Er wird auf jeden Fall freigegeben.

=head2 Argumente
=over 3
=item buf
Puffer mit zu substituierenden Werten, muss dynamisch alloziert sein,
da er in dieser Funktion freigegeben wird.
=item num
Anzahl der Eintraege im Feld C<sub>
=item sub
Name/Wert-Paare fuer die Substitution. Wird C<@name@> im Puffer
gefunden, so wird dieser durch den entsprechenden Wert ersetzt.
=item new
Zeiger auf Puffer. Dieser wird dynamisch alloziert und muss vom
Aufrufer freigegeben werden.
=back

=head2 Rueckgabewert
liefert RETURN_ACCEPTED wenn alles ok (dann ist auch C<new> gueltig),
oder RETURN_ERROR.
Fehler werden ins Library-Logfile geschrieben.
*/
PRIVATE int prn_Substitute(char *buf,int num,SubstRec sub[],char **new,char esc_char)
{
	int i,len,newlen;
	char *op,*sp;
	char *tmp;
    int lines;
	
	*new=NULL;
	
	if((buf==NULL)&&(buf[0]=='\0')) {
		return(RETURN_ACCEPTED);
	}

	lines=0;
	len=strlen(buf);
	
	/* we need a buffer that is large enough to hold the original and
	   all the subsitutions. Say we need about 50 bytes per
	   substitution */
	newlen=len+(50*num);
	tmp=malloc(newlen);
	if(tmp==NULL) {
		LogPrintf(FAC_PRN,LT_ERROR,
				  "Fehler beim Malloc fuer neuen Puffer");
		return(RETURN_ERROR);
	}
    memset(tmp,0,newlen);
	sp=op=buf;
	tmp[0]='\0';
	
	while(*op) {
        if(*op=='\n') {
            lines++;
        }
		if(*op==esc_char) {
			/* first copy up to this space */
			*op='\0';
			strcat(tmp,sp);
			
			/* now find out about this identifier */
			sp= ++op;
			while(*op && (*op!=esc_char)) {
                if(*op=='\n') {
                    lines++;
                }
				++op;
			}
			*op='\0';
			for(i=0;i<num;i++) {
				if(strcmp(sub[i].var,sp)==0) {
					break;
				}
			}
			if(i<num) {
				strcat(tmp,sub[i].val);
				if(strlen(tmp)>newlen) {
					LogPrintf(FAC_PRN, LT_ERROR,
							  "Neue Stringlaenge zu kurz in %d.",
                              lines);
					free(buf);
					return(RETURN_ERROR);
					
				}
			} else {
				LogPrintf(FAC_PRN,LT_ERROR,
						  "Fehler beim Subst von '%s' in Zeile %d.\n"
                          " Variable nicht vorhanden",
                          sp,lines);
				free(buf);
				free(tmp);
				return(RETURN_ERROR);
			}
			sp=op+1;				
		}
		++op;
	}
	strcat(tmp,sp);	

	free(buf);
	*new=tmp;
	return(RETURN_ACCEPTED);
}

/* [endfold] */
/* [[ prn_ReadAndSub ]] */

/*
=head1 Lesen einer Datei und Ersetzen von Strings
=head2 Signatur
   

=head2 Beschreibung
Liefert einen Puffer mit dem Inhalt einer Datei, wobei 
alle Vorkommnisse der durch C<@xxx@> eingegrenzten Strings durch die
jeweiligen Variablen aus C<sub> ersetzt worden sind.

=head2 Argumente
=over 3
=item fname
Name der zu lesenden Datei
=item num
Anzahl der Eintraege im Feld C<sub>
=item sub
Name/Wert-Paare fuer die Substitution. Wird C<@name@> im Puffer
gefunden, so wird dieser durch den entsprechenden Wert ersetzt.
=item buf
Zeiger auf Puffer. Dieser wird dynamisch alloziert und muss vom
Aufrufer freigegeben werden.
=back

=head2 Rueckgabewert
liefert RETURN_ACCEPTED wenn alles ok (dann ist auch C<buf> gueltig),
oder RETURN_ERROR.
Fehler werden ins Library-Logfile geschrieben.
*/
int prn_ReadAndSub(char *fname,
						   int num,SubstRec sub[],
						   char **buf,char esc_char)
{
	
	char *obuf1,*obuf2;
	if(fname==NULL) {
		LogPrintf(FAC_PRN,LT_ERROR,
				  "Dateiname ist leer.");
		return(RETURN_ERROR);
	}
	
	if(prn_ReadFile(fname,&obuf1)!=RETURN_ACCEPTED) {
		return(RETURN_ERROR);
	}
	if(prn_Conditions(obuf1,num,sub,&obuf2)!=RETURN_ACCEPTED) {
		return(RETURN_ERROR);
	}
	if(prn_Substitute(obuf2,num,sub,buf,esc_char)!=RETURN_ACCEPTED) {
		return(RETURN_ERROR);
	}
	
	return(RETURN_ACCEPTED);
}

/* [endfold] */
/* [[ prn_ReadSubAndWrite ]] */

/*
=head1 Lesen, Ersetzen und Schreiben einer neuen Datei
=head2 Signatur
   

=head2 Beschreibung
Liest eine Datei in einen internen Puffer, wobei 
alle Vorkommnisse der durch C<@xxx@> eingegrenzten Strings durch die
jeweiligen Variablen aus C<sub> ersetzt werden und schreibt diesen
Puffer in eine neue Datei.

=head2 Argumente
=over 3
=item rname
Name der zu lesenden Datei
=item num
Anzahl der Eintraege im Feld C<sub>, kann auch <=0 sein, dann ist das
Feld NULL-terminiert.
=item sub
Name/Wert-Paare fuer die Substitution. Wird C<@name@> im Puffer
gefunden, so wird dieser durch den entsprechenden Wert ersetzt.
=item wname
Name der zu schreibenden Datei
=back

=head2 Rueckgabewert
liefert RETURN_ACCEPTED wenn alles ok (dann ist auch eine neue Datei angelegt),
oder RETURN_ERROR.
Fehler werden ins Library-Logfile geschrieben.
*/
/*PRIVATE*/ int prn_ReadSubAndWrite(char *rname,
								int num,SubstRec sub[],
								char *wname,char esc_char)
{
	char *buf;
	int fd;
	int len;
	int i;
    int rv;
	
	if(num<=0) {
		for(i=0;sub[i].var!=NULL;i++);
		num=i;
	}

	if(prn_ReadAndSub(rname,num,sub,&buf,esc_char)!=RETURN_ACCEPTED) {
		return(RETURN_ERROR);
	}

	fd=open(wname,O_WRONLY|O_CREAT,0660);
	if(fd<0) {
		LogPrintf(FAC_PRN,LT_ERROR,
				  "Kann Drucker-Ausgabedatei '%s' nicht oeffnen", wname);
		return(RETURN_ERROR);
	}
	len=strlen(buf);
	if(write(fd,buf,len)!=len) {
		LogPrintf(FAC_PRN,LT_ERROR,
				  "Fehler beim Schreiben von '%s'.", wname);
		free(buf);
		close(fd);
		return(RETURN_ERROR);
	}
	free(buf);
	rv=close(fd);
    if(rv!=0) {
        LogPrintf(FAC_PRN,LT_ERROR,
                  "Fehler beim Schliessen von '%s' - %s.",
                  wname,strerror(errno));
        return(RETURN_ERROR);
    }
    
    return(RETURN_ACCEPTED);
}

/* [endfold] */ 


/* RETURN ACCEPTED if printer exists */
/*PUBLIC int Prn_LaserPrinterExists(char *name)
{
	int rv=RETURN_ACCEPTED;
	printerCap prn;
	
	rv=Prn_GetLaserPrinter(name,&prn);
	
	return rv;
}*/



/* [[ prn_calcMod10_CheckDigit ]] */

PRIVATE int prn_calcMod10_CheckDigit(char *str)
{
	int	i,
		factor = 3,
		sum = 0;

	if (str == NULL || strlen(str) < 1)
		return -1;
	for (i = strlen(str)-1; i >= 0; i--) {
		if (!isdigit ((int) str[i]))
			return -1;
		sum += (str[i] - '0') * factor;
		factor = (factor == 3) ? 1 : 3;
	}
	sum = (10 - (sum % 10)) % 10;

	return sum;
}

/* [endfold] */

/* [[ prn_EAN_Tray ]] */

/*
=head1 Etikett Normalware drucken
=head2 Signatur
   

=head2 Beschreibung
Erzeugt eine Datei mit den Daten fuer ein Etikett fuer Normalware.

=head2 Argumente
=over 3
=item tmpfile
Name der zu schreibenden Datei
=item size
=item mmdots
=item printmode
mit oder ohne cutter (?)
=item etikett
Daten, die aufs Etikett gedruckt werden sollen.
=back

=head2 Rueckgabewert
liefert einen FILE-Pointer als Rueckgabewert (aus historischen Gruenden),
der bei einem Fehler NULL ist, sonst irgendwas anderes (aber auf jeden
Fall kein offenes File).
Fehler werden ins Library-Logfile geschrieben.
*/

PRIVATE int prn_EAN_Tray(char *tmpfile, char *filename,
			Prn_EtikettData etikett, char esc_char,int conversion)
{
	char ean13[500];
	char ean128[50];	
	char charge[100];
	char buf[20];
	char buf1[20];
	char buf2[5];
	char pz[5];

	char qty[20];
	char date[50];
	char mhdstrg[50];
	char gewicht[20];
	char gewstrg[50];
	char teid[50];
	char gewflag[2];
	char normflag[2];

	SubstRec s[]={
	{"Title",NULL},
	{"ArtBez",NULL},
	{"Ean13",NULL},
	{"Menge",NULL},
	{"Einheit",NULL},
	{"MhdDate",NULL},
	{"MhdStrg",NULL},
	{"Charge",NULL},
	{"Gewicht",NULL},
	{"GewStrg",NULL},
/* mandant t.*/
	{"Mandant",NULL},
	{"ArtNr",NULL},
	{"Ean128",NULL},	
	{"NORMAL",NULL},
	{"GEW",NULL},
	NULL
	};
	
	s[0].val=cv_ansi2ibm(etikett->title,conversion);
	s[1].val=cv_ansi2ibm(etikett->artbez,conversion);
	s[2].val=ean13;
	s[3].val=qty;
	s[4].val=cv_ansi2ibm(etikett->einheit,conversion);
	s[5].val=date;
	s[6].val=mhdstrg;
/*	s[7].val=charge;*/
	s[7].val=cv_ansi2ibm(etikett->charge,conversion);
	s[8].val=gewicht;
	s[9].val=gewstrg;
/* mandant t.*/
	s[10].val=cv_ansi2ibm(etikett->mandant,conversion);
	s[11].val=cv_ansi2ibm(etikett->artnr,conversion);
	s[12].val=ean128;
	s[13].val=normflag;
	s[14].val=gewflag;

	if ( strlen(etikett->ean) <= 13) {
//		LogPrintf(FAC_PRN,LT_NOTIFY,"prn_EAN_Tray, ean13 vor calc mit ean %s",etikett->ean);
		strncpy(buf1,etikett->ean,12);
	    buf1[12] = '\0';

		sprintf(ean13,"%s%d", buf1, prn_calcMod10_CheckDigit(etikett->ean));
		ean13[13] = '\0';
	} else { /* strlen(ean) sollte hier 14 sein, kein checkdigit nötig  */
//		LogPrintf(FAC_PRN,LT_NOTIFY,"prn_EAN_Tray, ean14 vor calc mit ean %s",etikett->ean);
		strcpy(buf1,etikett->ean);
		sprintf(ean13,"%s", buf1);
		ean13[14] = '\0';
	}
//	LogPrintf(FAC_PRN,LT_NOTIFY,"prn_EAN_Tray, ean13 nach calc mit ean %s",ean13);
	
	sprintf(qty,"%0.4ld",etikett->qty);

	sprintf(date,"%02d.%02d.%04d",
		etikett->best_before.v[OWDATE_DAY],
		etikett->best_before.v[OWDATE_MON],
		etikett->best_before.v[OWDATE_YEAR]);

	sprintf(gewicht,"%.3f kg",etikett->nettoweight/1000.0);
	sprintf(gewstrg,"%06ld",etikett->nettoweight);

	sprintf(mhdstrg,"%02d%02d%02d",
		etikett->best_before.v[OWDATE_YEAR] % 100,
		etikett->best_before.v[OWDATE_MON],
		etikett->best_before.v[OWDATE_DAY]);

	sprintf(charge,"%0.4s", etikett->charge);
	charge[4] = '\0';

	memset(ean128,0,sizeof(ean128));

    /* mgu, wenn ean13 = 14 stellig, dann nichts berechnen */

//        LogPrintf(FAC_PRN,LT_NOTIFY,"prn_EAN_Tray, ean128 alt vor calc mit ean %s",etikett->ean);
		if (etikett->gewart) {
            buf[0] = '9';
            sprintf(gewflag,"1");
            sprintf(normflag,"0");
        } else {
            buf[0] = '0';
            sprintf(gewflag,"0");
            sprintf(normflag,"1");
        }
    if ( strlen(etikett->ean) <= 13)
	{
		strcpy(buf+1,etikett->ean);
    	sprintf(ean128,"%s%d",buf, prn_calcMod10_CheckDigit(buf));
    } else {
//        LogPrintf(FAC_PRN,LT_NOTIFY,"prn_EAN_Tray, ean128 neu vor calc mit ean %s",etikett->ean);
		strcpy(buf,etikett->ean);
    	sprintf(ean128,"%s",buf);
    }
//    LogPrintf(FAC_PRN,LT_NOTIFY,"prn_EAN_Tray, ean128 nach calc mit ean %s",ean128);


/* the old way

	if (etikett->gewart) {
		buf[0] = '9';
		sprintf(gewflag,"1");
		sprintf(normflag,"0");
	} else {
		buf[0] = '0';
		sprintf(gewflag,"0");
		sprintf(normflag,"1");
	}
    strcpy(buf+1,etikett->ean);
    sprintf(ean128,"%s%d",buf, prn_calcMod10_CheckDigit(buf));
*/	
	return (prn_ReadSubAndWrite(filename,0,s,tmpfile,esc_char));
}

/* [endfold] */
/* [[ prn_EAN_Pal ]] */

PRIVATE int prn_EAN_Pal(char *tmpfile, char *filename,
			Prn_EtikettData etikett, char esc_char,int conversion)
{

	char ean13[50];
	char ean128[50];
	char buf[100];
	char sscc[TEID_LEN+1];
	char sscc1[2];
	char sscc2[PRN_BBN_LEN+1];
	char sscc3[PRN_INTNR_LEN+1];
	char sscc4[PRN_TEID_LEN+1];
	char sscc5[2];

	char qty[20];
	char date[50];
	char mhdstrg[50];
	char gewstrg[50];	
	char gewicht[20];
	char teid[50];
	char gpflag[2];
	char gewflag[2];
	char normflag[2];		
	char *ptr;
	
	SubstRec s[]={
	{"Title",NULL},
	{"ArtBez",NULL},
/* mandant t.*/
	{"Mandant",NULL},
	{"ArtNr",NULL},	
	{"Ean13",NULL},
	{"Menge",NULL},
	{"Einheit",NULL},
	{"MhdDate",NULL},
	{"Charge",NULL},
	{"SSCC1",NULL},
	{"SSCC2",NULL},
	{"SSCC3",NULL},
	{"SSCC4",NULL},
	{"SSCC5",NULL},
	{"Gewicht",NULL},
	{"Ean128",NULL},
	{"MhdStrg",NULL},
	{"GewStrg",NULL},	
	{"GP",NULL},	
	{"GPEan",NULL},
	{"NORMAL",NULL},
	{"GEW",NULL},
	{"KName",NULL},
	{"KAdresse",NULL},
	{"KAdresse1",NULL},
	NULL
	};
/* mandant t.*/
	s[0].val=cv_ansi2ibm(etikett->title,conversion);
	s[1].val=cv_ansi2ibm(etikett->artbez,conversion);
	s[2].val=cv_ansi2ibm(etikett->mandant,conversion);
	s[3].val=cv_ansi2ibm(etikett->artnr,conversion);
	s[4].val=ean13;
	s[5].val=qty;
	s[6].val=cv_ansi2ibm(etikett->einheit,conversion);
	s[7].val=date;
	s[8].val=cv_ansi2ibm(etikett->charge,conversion);
	s[9].val=sscc1;
/* wir sollten eher die Daten der gesamten TE-Id nehmen ...
	s[10].val=etikett->bbn_number;
	s[11].val=etikett->int_number;
	s[12].val=teid;
	s[13].val=pz;
*/	
	s[10].val=sscc2;
	s[11].val=sscc3;
	s[12].val=sscc4;
	s[13].val=sscc5;
	s[14].val=gewicht;
	s[15].val=ean128;
	s[16].val=mhdstrg;
	s[17].val=gewstrg;	
	s[18].val=gpflag;
	s[19].val=cv_ansi2ibm(etikett->gpean,conversion);
	s[20].val=normflag;
	s[21].val=gewflag;

	s[22].val=cv_ansi2ibm(Var_GetString("EtikettKundenName",""),conversion);
	s[23].val=cv_ansi2ibm(Var_GetString("EtikettKundenAdr",""),conversion);
	s[24].val=cv_ansi2ibm(Var_GetString("EtikettKundenAdr1",""),conversion);

	if ( strlen(etikett->ean) <= 13) {
//		LogPrintf(FAC_PRN,LT_NOTIFY,"prn_EAN_Pal, ean13 vor calc mit ean %s",etikett->ean);
		strncpy(buf,etikett->ean,12);
	    buf[12] = '\0';

		sprintf(ean13,"%s%d", buf, prn_calcMod10_CheckDigit(etikett->ean));
		ean13[13] = '\0';
	} else { /* strlen(ean) sollte hier 14 sein, kein checkdigit nötig */
//		LogPrintf(FAC_PRN,LT_NOTIFY,"prn_EAN_Pal, ean14 vor calc mit ean %s",etikett->ean);
		strcpy(buf,etikett->ean);
		sprintf(ean13,"%s", buf);
		ean13[14] = '\0';
	}
//	LogPrintf(FAC_PRN,LT_NOTIFY,"prn_EAN_Pal, ean13 nach calc mit ean %s",ean13);

	sprintf(qty,"%0.4ld",etikett->qty);

	sprintf(date,"%02d.%02d.%02d",
		etikett->best_before.v[OWDATE_DAY],
		etikett->best_before.v[OWDATE_MON],
		etikett->best_before.v[OWDATE_YEAR] % 100);

	sprintf(gewicht,"%.1f",etikett->nettoweight/1000.0);
	sprintf(gewstrg,"%06ld",etikett->nettoweight);
	sprintf(mhdstrg,"%02d%02d%02d",
		etikett->best_before.v[OWDATE_YEAR] % 100,
		etikett->best_before.v[OWDATE_MON],
		etikett->best_before.v[OWDATE_DAY]);

	memset(sscc2,0,sizeof(sscc2));
	memset(sscc3,0,sizeof(sscc3));
	memset(sscc4,0,sizeof(sscc4));
	
/*	sprintf(sscc1,"%s",PRN_SSCC_PACKKZ);*/
	sprintf(sscc1,"%c",etikett->sscc[0]);
	sscc1[1] = '\0';
	ptr=etikett->sscc+1;
	
/*	sprintf(teid,"%0.*ld",PRN_TEID_LEN, etikett->teid);*/

	strncpy(sscc2,ptr,PRN_BBN_LEN);
	ptr+=PRN_BBN_LEN;
	strncpy(sscc3,ptr,PRN_INTNR_LEN);
	ptr+=PRN_INTNR_LEN;
	strncpy(sscc4,ptr,PRN_TEID_LEN);
	sprintf(sscc5,"%d", prn_calcMod10_CheckDigit(etikett->sscc));
	
/*	sprintf(sscc,"%s%s%s%s",sscc1, sscc2,sscc3, sscc4);
	sprintf(sscc5,"%d", prn_calcMod10_CheckDigit(sscc));
	sprintf(charge,"%0.4s", etikett->charge);
	charge[4] = '\0';
*/
	/* EAN 128 berechnen */

	memset(buf,0,sizeof(buf));

	/* mgu, wenn ean13 = 14 stellig, dann nichts berechnen */

//		LogPrintf(FAC_PRN,LT_NOTIFY,"prn_EAN_Tray, ean128 alt vor calc mit ean %s",etikett->ean);
		if (etikett->gewart) {
			buf[0] = '9';
			sprintf(gewflag,"1");
			sprintf(normflag,"0");
		} else {
			buf[0] = '0';
			sprintf(gewflag,"0");
			sprintf(normflag,"1");
		}
	if ( strlen(etikett->ean) <= 13)
	{
		strcpy(buf+1,etikett->ean);
    	sprintf(ean128,"%s%d",buf, prn_calcMod10_CheckDigit(buf));
	} else {
//	    LogPrintf(FAC_PRN,LT_NOTIFY,"prn_EAN_Tray, ean128 neu vor calc mit ean %s",etikett->ean);
		strcpy(buf,etikett->ean);
    	sprintf(ean128,"%s",buf);
	}
//	LogPrintf(FAC_PRN,LT_NOTIFY,"prn_EAN_Tray, ean128 nach calc mit ean %s",ean128);


/* the old way

    if (etikett->gewart) {
        buf[0] = '9';
        sprintf(gewflag,"1");
        sprintf(normflag,"0");
    } else {
        buf[0] = '0';
        sprintf(gewflag,"0");
        sprintf(normflag,"1");
    }
    strcpy(buf+1,etikett->ean);
    sprintf(ean128,"%s%d",buf, prn_calcMod10_CheckDigit(buf));

*/
	if (etikett->gp)
		sprintf(gpflag,"1");
	else sprintf(gpflag,"0");
	
	return (prn_ReadSubAndWrite(filename,0,s,tmpfile,esc_char));
}

/* [endfold] */
/* [[ prn_get_DruckerTyp ]] */

/* holt den zu einem Drucker gehoerenden Typ aus der printtabl-Datei,
 liefert RETURN_FOUND/NOTFOUND/ERROR */
LOCAL int prn_get_DruckerTyp(const char *drucker,
                             const char *defTyp, /* Rueckgabewert, wenn
                                               nichts gefunden wird */
                             char *typ, /* RET: typ des Druckers, z.b ZEBRA */
                             char *esc_char /* RET:escape character */
                             
                             )
{
	FILE	*f;
	char 	buf[200];
	int 	rv,len;
	char	*ptr,*p;
	char    *path;
	char    *fileName;

	if (esc_char)
		*esc_char = '@';
    if(Util_IsEmptyStr(drucker)||(typ==NULL)) {
        LogPrintf(FAC_PRN,LT_ERROR,"Drucker/typ NULL");
        return RETURN_ERROR;
    }

    /* hoffentlich ist der Platz gross genug */
    strcpy(typ,defTyp);

	path=Var_GetString("Sprn.PrinttablPath","./etc");
    fileName=StrForm("%s/%s",path,
                     Var_GetString("Sprn.PrinttablName","printtabl"));
	f = fopen(fileName,"r");
	if(f==NULL) {
        LogPrintf(FAC_PRN,LT_ERROR,"Kann '%s' nicht lesen",fileName);
        return RETURN_ERROR;
    }

    len=strlen(drucker);
    rv=RETURN_NOTFOUND;
    
    while (fscanf(f,"%s",buf) != EOF) {
		LogPrintf(FAC_PRN,LT_ERROR,
				"[%s]",
				buf);
		if (buf[0]=='#')
			continue;
		ptr=strchr(buf,':');
		if (ptr) {
			ptr++;
		}
        if (ptr && strncmp(ptr,drucker,len)==0) {
            ptr=strchr(buf,':');
			if (ptr) {
				ptr++;
            	ptr=strchr(ptr,':');
            }
			if (ptr) {
				ptr++;
            	ptr=strchr(ptr,':');
            }
            if (ptr) {
            	ptr++;
            	ptr=strchr(ptr,':');
            }
            if (ptr) {
                ptr++;
	            p=strchr(ptr,':');
    	        if (p) {
	    	        *p = '\0';
	        	    p++;
	            	if (esc_char)
	            		*esc_char = *p;
	        	}
	            strcpy(typ,ptr);
	        }
            rv=RETURN_FOUND;
            break;
        }
    }
    fclose(f);

    if(rv!=RETURN_FOUND) {
        LogPrintf(FAC_PRN,LT_ERROR,"Kein Drucker '%s' in %s",
                  drucker,fileName);
        /* kein Fehler, wir nehmen den Default */
    }
        
	return RETURN_ACCEPTED;
}

/* [endfold] */
/* [[ prn_Begleitschein ]] */

/*
=head1 Etikett Begleitschein drucken
=head2 Signatur
   

=head2 Beschreibung
Erzeugt eine Datei mit den Daten fuer ein Begleitscheinetikett

=head2 Argumente
=over 3
=item tmpfile
Name der zu schreibenden Datei
=item bd
Daten, die aufs Etikett gedruckt werden sollen.
=back

=head2 Rueckgabewert
Fehler werden ins Library-Logfile geschrieben.
*/

PRIVATE int prn_Begleitschein(char *tmpfile, char *filename,
			Prn_BegleitscheinData bd, char esc_char,int conversion)
{
/*
	char lhid[10];
	char bc_lhid[10];
	*/
	char hoehe[20];
	char date[15];
	char sscc[50];
	char *title;
	
	SubstRec s[]={
		{"Title",NULL},
		{"LhId",NULL},
		{"BcLhId",NULL},
		{"KomArt",NULL},
		{"Hoehe",NULL},		
		{"Tor",NULL},
		{"Tour",NULL},
		{"Kom",NULL},
		{"Date",NULL},
		{"Kunde1",NULL},
		{"Kunde2",NULL},
		{"Kunde3",NULL},
		{"Auftr",NULL},
		{"Info",NULL},		
/* sis: Lieferbereichs-Info */
		{"LiefberInfo",NULL},
		NULL
	};

	Var_Refresh();
	title=cv_ansi2ibm(Var_GetString("Sprn.BegleitTitle","LH - Begleitschein"),conversion);
	
	s[0].val=title;
/*	s[1].val=lhid;
	s[2].val=bc_lhid;
*/
	s[1].val=cv_ansi2ibm(bd->lhid,conversion);
	s[2].val=sscc;
	s[3].val=cv_ansi2ibm(bd->komart,conversion);
	s[4].val=hoehe;
	s[5].val=cv_ansi2ibm(bd->tor,conversion);
	s[6].val=cv_ansi2ibm(bd->tour,conversion);
	s[7].val=cv_ansi2ibm(bd->kom,conversion);
	s[8].val=date;
	s[9].val=cv_ansi2ibm(bd->kunde1,conversion);
	s[10].val=cv_ansi2ibm(bd->kunde2,conversion);
	s[11].val=cv_ansi2ibm(bd->kunde3,conversion);
	s[12].val=cv_ansi2ibm(bd->auftrnr,conversion);
	s[13].val=cv_ansi2ibm(bd->reserve,conversion);	
/* sis: Lieferbereichs-Info */
	s[14].val=cv_ansi2ibm(bd->liefinfo,conversion);

/*	sprintf(lhid,"%*ld", LHID_LEN,bd->lhid);
	sprintf(bc_lhid,"%*ld", LHID_LEN,bd->lhid);	
	*/

	sprintf(sscc,"%s%d", bd->lhid,prn_calcMod10_CheckDigit(bd->lhid));
	
	sprintf(hoehe,"%d cm",bd->hoehe/10);		
	sprintf(date,"%02d.%02d.%04d",
			bd->datum.v[OWDATE_DAY], bd->datum.v[OWDATE_MON],
			bd->datum.v[OWDATE_YEAR]);

	return (prn_ReadSubAndWrite(filename,0,s,tmpfile,esc_char));
}

/* [endfold] */

/* [[ Prn_PrintEanLabel ]] */

int Prn_PrintEANLabel(long type, const char *printer, Prn_EtikettData ed)
{
	char *tmpfile;
	char sysbuf[80];
	char *tmp_path;
	char typ[20];
	char *infile;
	char *fileName;
    char extra[20];
	char *path;
	char esc_char;
	int rv;
	int noansi_exists=RETURN_NOTFOUND;
	char *noansi=NULL;

	Var_Refresh();
	tmp_path=Var_GetString("Sprn.TmpPath","/tmp");
	tmpfile = tempnam(tmp_path,PRN_FILENAME);
	rv=prn_get_DruckerTyp(printer,"ZEBRA",typ,&esc_char);
		
	path=Var_GetString("Sprn.LabelPfad","./layout");
	
	noansi=Var_GetString("Sprn.NoAnsiConversion","MARK");
	noansi_exists=Var_ExistsListString(noansi,typ,",");
		
	if (type & PRN_MODE_TRAY) {
		infile=Var_GetString("Sprn.TrayLayout","ean_tray");
		
		fileName=StrForm("%s/%s-%s.bcf",path,typ,infile);        
		
		
		rv=prn_EAN_Tray(tmpfile,fileName,ed, esc_char,noansi_exists);
	} else if (type & PRN_MODE_PAL) {
#if 0
        extra[0]='\0';
		if (type & PRN_MODE_90DEG)
			strcat(extra,"90");
		if (type & PRN_MODE_CUTTER)
			strcat(extra,"CUT");
		if (ed->gp==1) {
			infile=Var_GetString("Sprn.GPPalLayout","ean_gpte");
        } else {
            infile=Var_GetString("Sprn.PalLayout","ean_te");
        }

		fileName=StrForm("%s/%s%s-%s.bcf",path,typ,extra,infile);
#endif
        infile=Var_GetString("Sprn.PalLayout","ean_te");		
		fileName=StrForm("%s/%s-%s.bcf",path,typ,infile);
		
		rv=prn_EAN_Pal(tmpfile,fileName,ed,esc_char,noansi_exists);		
	}
	if (rv==RETURN_ACCEPTED) {
#ifdef _HPUX_SOURCE
		sprintf(sysbuf,"lp -d%s %s",printer,tmpfile);
#else
/*		sprintf(sysbuf,"lpr -r -P%s %s",printer,tmpfile);*/
		sprintf(sysbuf,"lpr -h -P%s %s",printer,tmpfile);
					printf("%s\n",tmpfile);

#endif
		rv = system(sysbuf);
#ifdef _HPUX_SOURCE
		remove(tmpfile);
#endif
		rv=RETURN_ACCEPTED;
	}
	return rv;
}

/* [endfold] */
/* [[ Prn_PrintBegleitetikett ]] */

/* Wenn printself==0, wird nur das Tempfile in filename zurueckgeliefert.
*/

PUBLIC int Prn_PrintBegleitetikett(char *printer,Prn_BegleitscheinData bd,
									int printself, char *filename)
{
    char sysbuf[80];
	char *tmpfile;
	char *tmp_path;
	char typ[20];
	char *infile;
	char *fileName;
	char *path;
	char esc_char;
	int rv;
	char *ansi=NULL;
	int ansi_exists=RETURN_NOTFOUND;

	tmp_path=Var_GetString("Sprn.TmpPath","/tmp");
	tmpfile = tempnam(tmp_path,PRN_FILENAME);
	rv=prn_get_DruckerTyp(printer,"ZEBRA",typ,&esc_char);
		
	path=Var_GetString("Sprn.LabelPfad","./layout");
	infile=Var_GetString("Sprn.BegleitscheinLayout","begleit");
    fileName=StrForm("%s/%s-%s.bcf",path,typ,infile);
	
	ansi=Var_GetString("Sprn.NoAnsiConversion","MARK");
	ansi_exists=Var_ExistsListString(ansi,typ,",");
	
	rv=prn_Begleitschein(tmpfile,fileName,bd,esc_char,ansi_exists);
	
	if (rv==RETURN_ACCEPTED) {
		if (printself) {
#ifdef _HPUX_SOURCE
			sprintf(sysbuf,"lp -d%s %s",printer,tmpfile);
#else
/*			sprintf(sysbuf,"lpr -r -P%s %s",printer,tmpfile);*/
			sprintf(sysbuf,"lpr -h -P%s %s",printer,tmpfile);
			printf("%s\n",tmpfile);
#endif
			rv = system(sysbuf);
#ifdef _HPUX_SOURCE
			remove(tmpfile);
#endif
		} else if (filename!=NULL) {
			strcpy(filename,tmpfile);
		}
		rv=RETURN_ACCEPTED;		
	}

	return rv;
}

/* [endfold] */

/* [[ prn_FindEanType ]] */

PRIVATE int prn_FindEanType(EAN *eanbuf,int maxean,int typ, int maetyp)
{
	int rv	= RETURN_ERROR;
	int i;
	int found;
	
	if (eanbuf) {
		for (i=0;i<maxean;i++) {
			found	= 0;
			if (typ>=0) {
				found	+= (typ==eanbuf[i].eanTyp)?1:0; 
			} else {
				found++;
			}
			if (maetyp>=0) {
				found	+= (maetyp==eanbuf[i].eanMaeTyp)?1:0; 
			} else {
				found++;
			}
			if (found==2) {
				rv	= i;
				break;
			}
		}
	}
	return rv;
}

/* [endfold] */
/* [[ Prn_FillEANData ]] */

PUBLIC int Prn_FillEANData(long type, TEP *tep,ART *art,
						   EAN *eanbuf,int eancnt,
						   Prn_EtikettData ed)
{
	OwDateTimeRec	dtr;
	char 			steid[TEID_LEN+1];
	char 			*bbn;
	int				rv	= RETURN_ERROR;
	int				iean=-1,igpean=-1;
	EAN				*ean;
	char 			*in, *index;
	
	Var_Refresh();
  	bbn = Var_GetString("BetriebsNum",NULL);
  	in 	= Var_GetString("InterneNum",NULL);
	if (tep&&art&&eanbuf&&ed&&bbn) {
		memset(ed,0,sizeof(Prn_EtikettDataRec));
		rv	= RETURN_ACCEPTED;

		if (Util_IsEmptyStr(tep->tepTeId))
			return RETURN_ERROR;
			
		if (type & PRN_MODE_TRAY) {
			iean=prn_FindEanType(eanbuf,eancnt,EANTYP_LE2,-1);
			if (iean<0||eanbuf[iean].eanMaeTyp==MAETYP_TT) {
				iean=prn_FindEanType(eanbuf,eancnt,EANTYP_EVE,-1);
				if (iean<0)
					return RETURN_ERROR;
			}
		} else if (type & PRN_MODE_PAL) {
			iean=prn_FindEanType(eanbuf,eancnt,-1,MAETYP_TT);	
			if (iean<0)
				iean=prn_FindEanType(eanbuf,eancnt,EANTYP_LE2,MAETYP_PK);
			if (iean<0 ||
				tep->tepUnit.unitMenge%eanbuf[iean].eanUnit.unitMenge)

				iean=prn_FindEanType(eanbuf,eancnt,EANTYP_EVE,MAETYP_PK);
#if 0
			for (iean=0; iean<eancnt&&
						 (eanbuf[iean].eanMaeTyp!=MAETYP_PK||
								_invalid_ean(eanbuf[iean].eanEAN)||
								tep->tepUnit.unitMenge%eanbuf[iean].eanUnit.unitMenge);
						iean++);
#endif
		
			if (iean<0)
				return RETURN_ERROR;

			igpean = prn_FindEanType(eanbuf,eancnt,-1,MAETYP_GP);
			if (igpean<0)		
				igpean = prn_FindEanType(eanbuf,eancnt,-1,MAETYP_TT);

			/*	
			* wenn ganzpaletten/-ttmenge, dann zusaetzlich gp/tt-ean
			*/
			
			if (igpean>=0) {
				if (eanbuf[igpean].eanUnit.unitMenge==tep->tepUnit.unitMenge) {
					strncpy(ed->gpean,eanbuf[igpean].eanEAN,ARTNR_LEN);
					ed->gp=1;
				}
			}
		}

		ean	= &eanbuf[iean];
		strncpy(ed->title,Var_GetString("Sprn.EtikettTitle","Testetikett"),PRN_TITLE_LEN);
		strncpy(ed->sscc,tep->tepTeId,TEID_LEN);
		strncpy(ed->bbn_number,bbn,PRN_BBN_LEN);
		strncpy(ed->artbez,art->artArtBez,ARTBEZ_LEN);
/* mandant t.*/
		strncpy(ed->mandant,art->artMandant,MANDANT_LEN);
		strncpy(ed->artnr,art->artArtNr,12);
	
		if ( strlen(ean->eanEAN) <= 13) {	
//			LogPrintf(FAC_PRN,LT_NOTIFY,"Prn_FillEANData, ean13 mit ean %s",ean->eanEAN);
			strncpy(ed->ean,ean->eanEAN,12);
		} else {
//			LogPrintf(FAC_PRN,LT_NOTIFY,"Prn_FillEANData, ean14 mit ean %s",ean->eanEAN);
			strcpy(ed->ean,ean->eanEAN);
		}

		strncpy(ed->int_number,in,PRN_INTNR_LEN);
		if (art->artArtAttr&ARTATTR_GEWART) {
			ed->gewart	= 1;
		}
		ed->nettoweight	= tep->tepUnit.unitGewNetto;
				
		strncpy(ed->einheit,ean->eanVol.volumeEinheit,EINHEIT_LEN);		

		if (type & PRN_MODE_TRAY) {
			ed->qty	= ean->eanUnit.unitMenge;
		} else if (type & PRN_MODE_PAL) {			
			if (ean->eanUnit.unitMenge) {
				ed->qty	= tep->tepUnit.unitMenge/ean->eanUnit.unitMenge;
				if (tep->tepUnit.unitMenge%ean->eanUnit.unitMenge)
					ed->qty++;
			} else
				ed->qty	= tep->tepUnit.unitMenge;
		}
		

		OwDateTimeFromUxTime(&dtr,&tep->tepUnit.unitMHD);
		OwDateFromDateTime(&ed->best_before,&dtr);
		strncpy(ed->charge,tep->tepUnit.unitCharge,CHARGE_LEN);

		if (Util_IsEmptyStr(ed->sscc)==FALSE) {
			index = ed->sscc+10;
			strncpy(steid,index,strlen(ed->sscc)-10);
			steid[strlen(ed->sscc)-10]='\0';
			ed->teid	= atol(steid);
		}
	}
	return rv;
}

/* Function to give the date formated back */
void Prn_FormatDate (OwDateTime d,char *date) 
{
	sprintf(date,"%02d.%02d.%04d",
   			d->v[OWDATETIME_DAY],
			d->v[OWDATETIME_MON],
			d->v[OWDATETIME_YEAR]);
}	

/* Function to give the date formated back */
void Prn_FormatTime (OwDateTime d,char *time)
{
	sprintf(time,"%02d:%02d:%02d",
			d->v[OWDATETIME_HOUR],
			d->v[OWDATETIME_MIN],
			d->v[OWDATETIME_SEC]);
}	

/* Function to get the curremt Time and Date
 * as a formated string back
 * (date: DD.MM.JJJJ , time: HH:MM:SS)
 */
int Prn_GetTimeDate(char *ddate,char *ttime) 
{
	OwDateTimeRec d;
	time_t tnow=0;
	int rv=RETURN_ACCEPTED;
	

	if( Util_IsProject_P1E() )
	  {
		tnow = Util_GetP1E_Date();
	  } 
	else 
	  {
		tnow =time(NULL);
	  }

	if (!ddate || !ttime) {
		return RETURN_ERROR;
	}
	memset(ddate,0,sizeof(*ddate));
	memset(ttime,0,sizeof(*ttime));
	OwDateTimeFromUxTime(&d,&tnow);
	
	Prn_FormatDate(&d,ddate);
	Prn_FormatTime(&d,ttime);
	
	return rv;
}

char* Prn_GetLsPrinterByMandant(void *tid, char *recid )
{
  char *printer = NULL;
  char *fac = FAC_PRN;
  int rv = 0;
  char acMandant[MANDANT_LEN+1] = {0};
  char *buffer = NULL;

  CHECK_NULL_ARGS( recid, printer );

  rv = TExecSql( tid, 
				 "select mandant from " TN_MAK " where id=:a",
				 SELSTR( acMandant, MANDANT_LEN+1 ),
				 SQLSTR( recid, RECID_LEN+1 ),
				 NULL );

  if( rv < 0 && TSqlError( tid ) != SqlNotFound )
	{
	  LogPrintf( fac, LT_ALERT, 
				 "Datenbankfehler: %s mit recid: %s", 
				 TSqlErrTxt(tid),
				 recid );
	  return NULL;
	}
  else if( rv < 0  && TSqlError( tid ) == SqlNotFound ) 
	{
	  rv = TExecSql( tid, 
					 "select mandant from " TN_PMAK " where id=:a",
					 SELSTR( acMandant, MANDANT_LEN+1 ),
					 SQLSTR( recid, RECID_LEN+1 ),
					 NULL );

	  if( rv < 0 && TSqlError( tid ) != SqlNotFound )
		{
		  LogPrintf( fac, LT_ALERT, 
					 "Datenbankfehler: %s mit recid: %s", 
					 TSqlErrTxt(tid),
					 recid );
		  return NULL;
		}
	  else if( rv < 0  && TSqlError( tid ) == SqlNotFound ) 
		{
		  LogPrintf( fac, LT_ALERT, "Nix gefunden mit recid '%s'. Kein Auftrag in der MAK, oder in der PMAK",
		  	recid);
		  return NULL;
		}
	}

  buffer = StrCreate( StrForm( "Sprn.LiefDruckerForMandant%s", acMandant ) );

  if( buffer == NULL )
	buffer = "StrCreate FAILED!";

  LogPrintf( fac, LT_DEBUG, "Recid %s hat Mandant %s nehme Wert aus Parameter '%s'",
			 recid, acMandant, buffer );

  printer = Var_GetString( buffer, "" );

  LogPrintf( fac, LT_DEBUG, "Ermittelter Drucker ist: '%s'", printer );

  StrGiveup( buffer );

  if( Util_IsEmptyStr( printer ) )
	printer = NULL;

  return printer;
}

/* [endfold] */
#undef _print_c
/* EOF */

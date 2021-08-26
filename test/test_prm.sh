#/usr/bin/env bash

for i in prm/* ; do

	if ! test -d "$i" ; then
		continue;
	fi
	
	#echo "testing $i"
	
	if test -f "$i"/*.cc ; then
		FILE="$i"/*.cc
	fi
	if test -f "$i"/*.pdl ; then
		FILE="$i"/*.pdl
	fi
	if test -f "$i"/*.pds ; then
		FILE="$i"/*.pds
	fi
	if test -f "$i"/*.pl ; then
		FILE="$i"/*.pl
	fi	
	if test -f "$i"/*.h ; then
		FILE="$i"/*.h
	fi	
	if test -f "$i"/*.rc ; then
		FILE="$i"/*.rc
	fi	
	
	DIR=`dirname $FILE`
	DIR_LEN=${#DIR}	
	FILENAME=${FILE:$DIR_LEN+1}	
	
	rm -rf work
	mkdir work
	cp $FILE work	
	../replace work -prmget -doit 2>&1 > /dev/null
	res=`diff -u work/${FILENAME} ${FILE}.erg`
	
     if ! test -z "$res" ; then
            echo "DIFFER in $i:"
            echo "   $res"
            echo           
      else
            echo "check $i passed"
      fi
	 

done
 

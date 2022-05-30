#/usr/bin/env bash

for i in vamultiplemalloc/* ; do

	if ! test -d "$i" ; then
		continue;
	fi
	
	#echo "testing $i"
	
	if test -f "$i"/*.c ; then
		FILE="$i"/*.c
	fi
	
	DIR=`dirname $FILE`
	DIR_LEN=${#DIR}	
	FILENAME=${FILE:$DIR_LEN+1}	
	
	rm -rf work
	mkdir work
	cp -f $FILE work	
	../replace work -doit -vamulmalloc 2>&1 > /dev/null
	res=`diff -u work/${FILENAME} ${FILE}.erg`
	
     if ! test -z "$res" ; then
            echo "DIFFER in $i:"
            echo "   $res"
            echo           
      else
            echo "check $i passed"
      fi
	 

done
 

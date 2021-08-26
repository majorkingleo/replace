#/usr/bin/env bash

for i in implicit/* ; do

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
	cp $FILE work	
	cp $i/*.* work	
	../replace work -compile-log work/*.log -implicit -doit 2>&1 > /dev/null
	res=`diff work/${FILENAME} ${FILE}.erg`
	
     if ! test -z "$res" ; then
            echo "DIFFER in $i:"
            echo "   $res"
            echo           
      else
            echo "check $i passed"
      fi
	 

done
 

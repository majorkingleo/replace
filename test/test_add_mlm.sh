#/usr/bin/env bash


function do_test_case () 
{
	TESTCASE="${1##*/}";
	FILE=`ls $1/*.c`
##	DIR=`dirname $1`
	DIR="${1##/*/}"
	DIR_LEN=${#DIR}	
	FILENAME=${FILE:$DIR_LEN+1}	

#	echo "DIR: $DIR"
#	echo "DIR_LEN: $DIR_LEN"
#	echo "FILENAME: $FILENAME"
#	echo "FILE: $FILE"
	
	rm -rf work
	mkdir work
	cp $1/* work
#	echo ../replace work $2 2>&1
	shift
#	echo ../replace work $* 2>&1 
	../replace work $* 2>&1 > /dev/null

	res=`diff work/${FILENAME} work/${FILENAME}.erg`
	
    if ! test -z "$res" ; then
           echo "DIFFER in $TESTCASTE:"
           echo "   $res"
           echo           
    else
           echo "check $TESTCASE passed"
    fi
}

do_test_case add-mlm/t01 -add-mlm -function-name maskSetMessage -function-arg 3 -function-call MlMsg -doit
do_test_case add-mlm/t02 -add-mlm-wamas-box -doit
do_test_case add-mlm/t03 -add-mlm-wamas-box -doit
do_test_case add-mlm/t04 -add-mlm-wamas-box -doit
do_test_case add-mlm/t05 -add-mlm-wamas-box -doit
do_test_case add-mlm/t06 -add-mlm-wamas-box -doit
do_test_case add-mlm/t07 -add-mlm-wamas-box -doit
do_test_case add-mlm/t08 -add-mlm-wamas-box -doit
do_test_case add-mlm/t09 -add-mlm-wamas-box -doit

#!/usr/bin/env bash

if ! test -d work ; then
 mkdir work
fi

./test_remove_versid.sh
./test_primanlist.sh
./test_owcallback.sh
./test_mlm.sh
./test_restoreshell.sh
./test_vamultiplemalloc.sh
./test_genericpointer.sh
./test_unused_var.sh
./test_format-string.sh
./test_implicit.sh
./test_assign.sh
./test_sprintf.sh
./test_space.sh

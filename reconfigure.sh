#!/usr/bin/env bash
aclocal
automake --add-missing
automake -f
autoconf -f
./configure

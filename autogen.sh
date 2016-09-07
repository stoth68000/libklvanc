#!/bin/bash

case "$1" in
--clean)
	make distclean
	rm -rf autom4te.cache m4
	rm -f aclocal.m4 config.log config.status configure
	rm -f depcomp install-sh Makefile Makefile.in missing
	rm -f config.guess config.sub libtool ltmain.sh
	rm -f src/Makefile.in
	;;
--build)
	aclocal
	autoconf
	autoreconf -i
	automake --add-missing
	;;
*)
        echo "$0 --clean --build"
        exit 1
        ;;
esac


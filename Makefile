all: qtunet qtunet-qt4 mytunet

SVNREV = `svnversion -n | sed -e 's/[[:digit:]]*://g' | sed -e 's/M//g'`
VERSION = 0.1
FULLVER = ${VERSION}.${SVNREV}
DISTNAME = mytunet-${FULLVER}
DISTDIR = ${DISTNAME}
QT3_QMAKE = qmake
QT3_SPEC = freebsd-g++
QT3_DIR = /usr/local
QT4_QMAKE = qmake-qt4

qtunet:
	@echo "Building qtunet (Qt3 version)"
	cd src/qtunet ; ${QT3_QMAKE} -spec ${QT3_SPEC} ; make QTDIR=${QT3_DIR}

qtunet-qt4:
	@echo "Building qtunet (Qt4 version)"
	cd src/qtunet-qt4 ; ${QT4_QMAKE} ; make

mytunet:
	@echo "Building mytunet"
	cd src/mytunet-console/console ; ./genmakefile.sh ; make

install:
	@echo "Installing qtunet (Qt4 version)"
	cd src/qtunet-qt4 ; make install

.PHONY: pack clean install

clean:
	cd src/mytunet-console/console ; make clean
	cd src/qtunet-qt4 ; make clean
	cd src/qtunet ; make clean

pack:
	svn export . ${DISTDIR}
	rm -fdr ${DISTDIR}/release ${DISTDIR}/tmp ${DISTDIR}/web
	# ensure we have a clean copy
	cd ${DISTDIR}/src/qtunet-qt4 ; make clean
	cd ${DISTDIR}/src/qtunet ; make clean
	rm -f ${DISTDIR}/src/*.o
	tar zcvf ${DISTNAME}.tar.gz ${DISTDIR}
	rm -fdr ${DISTDIR}



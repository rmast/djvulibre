#C- This program is free software; you can redistribute it and/or
#C- modify it under the terms of the GNU General Public License,
#C- either Version 2 of the License or (at your option) any later
#C- version.  The license should have accompanied the program 
#C- or you may obtain a copy of the license from the Free Software
#C- Foundation at http://www.fsf.org.
#C-
#C- This program is distributed in the hope that it will be useful,
#C- but WITHOUT ANY WARRANTY; without even the implied warranty of
#C- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#C- GNU General Public License for more details.


SHELL = /bin/bash

PACKAGE_NAME = djvulibre
PACKAGE_VERSION = 3.5.24

srcdir = .
top_srcdir = ..
top_builddir = ..

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin
datarootdir = ${prefix}/share
datadir = ${datarootdir}
libdir = ${exec_prefix}/lib
mandir = ${datarootdir}/man

CC = gcc
CXX = g++
RM = /bin/rm -f
AR = /usr/bin/ar
LN_S = ln -s
TOUCH = /usr/bin/touch
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
LIBTOOL = $(SHELL) $(top_builddir)/libtool

DEFS = -DHAVE_CONFIG_H
OPTS = -DNDEBUG -Wall -O3 -Wno-non-virtual-dtor -mtune=i686
CXXRPOFLAGS = 
JPEG_LIBS = -ljpeg
JPEG_CFLAGS = 
TIFF_LIBS = -ltiff
TIFF_CFLAGS = 
THREAD_LIBS =  -lpthread
THREAD_CFLAGS =  -pthread -DTHREADMODEL=POSIXTHREADS

LIBDJVU = ${top_builddir}/libdjvu/libdjvulibre.la

MYSQLLIBS = -L/usr/lib/mysql -lmysqlclient
MYSQLFLAGS = -I/usr/include/mysql

INCS = -I${top_builddir} -I${top_srcdir} -I${top_srcdir}/libdjvu -I${srcdir}
FLAGS = ${DEFS} ${INCS} ${OPTS} ${JPEG_CFLAGS} ${TIFF_CFLAGS} ${THREAD_CFLAGS}
LIBS=  -lm 
MORELIBS=  ${TIFF_LIBS} -lm 
CFLAGS = ${FLAGS}   
CXXFLAGS = ${FLAGS} ${CXXRPOFLAGS} ${MYSQLFLAGS}   
SUBDIRS = jb2cmp

PROGRAMS = bzz c44 cjb2 cpaldjvu csepdjvu \
	   ddjvu djvm djvmcvt djvudump \
	   djvups djvuextract djvumake \
	   djvused djvutxt djvuserve \
	   inhdict exportshapes

SCRIPTS = djvudigital any2djvu

MAN1PAGES = djvu

all: ${PROGRAMS}

install: install-bin install-scripts install-man

install-bin: ${PROGRAMS} FORCE
	${INSTALL} -d ${DESTDIR}${bindir}
	for n in ${PROGRAMS} ; do \
	  ${LIBTOOL} --mode=install \
	    ${INSTALL_PROGRAM} $$n ${DESTDIR}${bindir} \
          || exit ; done

install-scripts: FORCE
	${INSTALL} -d ${DESTDIR}${bindir}
	for n in ${SCRIPTS} ; do \
	  ${INSTALL_PROGRAM} -m 0755 ${srcdir}/$$n ${DESTDIR}${bindir} \
	  || exit ; done

install-man: FORCE
	${INSTALL} -d ${DESTDIR}${mandir}/man1
	for n in ${PROGRAMS} ${SCRIPTS} ${MAN1PAGES} ; do \
	  if test -r ${srcdir}/$$n.1 ; then \
	    ${INSTALL_DATA} ${srcdir}/$$n.1 ${DESTDIR}${mandir}/man1 \
	    || exit ; fi ; done

depend: FORCE
	for d in ${SUBDIRS}; do ( cd $$d && ${MAKE} $@ ); done
	${CXX} -MM ${CXXFLAGS} ${srcdir}/*.cpp > Makefile.dep

clean: FORCE
	for d in ${SUBDIRS}; do ( cd $$d && ${MAKE} $@ ); done
	-${LIBTOOL} --mode=clean ${RM} 2>/dev/null ${PROGRAMS}
	-${RM} 2>/dev/null *.o *.a *.so *.stamp

distclean: clean
	for d in ${SUBDIRS}; do ( cd $$d && ${MAKE} $@ ); done
	-${RM} -r 2>/dev/null Makefile *.rpo ii_files

# Link

JB2OBJS= jb2tune.o jb2cmp/libjb2cmp.a

jb2cmp/libjb2cmp.a: FORCE
	cd jb2cmp && ${MAKE}

annotate: annotate.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} annotate.o ${LIBDJVU} ${LIBS}

bzz: bzz.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} bzz.o ${LIBDJVU} ${LIBS}

c44: c44.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} c44.o ${LIBDJVU} ${LIBS}

cjb2: cjb2.o ${JB2OBJS}
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} cjb2.o ${JB2OBJS} ${LIBDJVU} ${MORELIBS}

cpaldjvu: cpaldjvu.o ${JB2OBJS}
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} cpaldjvu.o ${JB2OBJS} ${LIBDJVU} ${LIBS}

csepdjvu: csepdjvu.o ${JB2OBJS}
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} csepdjvu.o ${JB2OBJS} ${LIBDJVU} ${LIBS}

ddjvu: ddjvu.o tiff2pdf.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} ddjvu.o tiff2pdf.o ${LIBDJVU} ${MORELIBS}

djvm: djvm.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} djvm.o ${LIBDJVU} ${LIBS}

djvmcvt: djvmcvt.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} djvmcvt.o ${LIBDJVU} ${LIBS}

djvudump: djvudump.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} djvudump.o ${LIBDJVU} ${LIBS}

djvups: djvups.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} djvups.o ${LIBDJVU} ${LIBS}

djvuextract: djvuextract.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} djvuextract.o ${LIBDJVU} ${LIBS}

djvumake: djvumake.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} djvumake.o ${LIBDJVU} ${LIBS}

djvused: djvused.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} djvused.o ${LIBDJVU} ${LIBS}

djvutxt: djvutxt.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} djvutxt.o ${LIBDJVU} ${LIBS}

djvuserve: djvuserve.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} djvuserve.o ${LIBDJVU} ${LIBS}

inhdict: inhdict.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} inhdict.o ${LIBDJVU} ${LIBS}

exportshapes: exportshapes.o
	${LIBTOOL} --mode=link \
	  ${CXX} -o $@ ${CXXFLAGS} -Wnouninitialized exportshapes.o ${LIBDJVU} ${LIBS} ${MYSQLLIBS}

${PROGRAMS}: ${LIBDJVU}

# Rules

FORCE:
.PHONY: FORCE
.SUFFIXES: .c .cpp .o

.c.o:
	${CC} ${CFLAGS} -c $<

.cpp.o:
	${CXX} ${CXXFLAGS} -c $<

.c.lo:
	${LIBTOOL} --mode=compile ${CC} ${CFLAGS} -c $<

.cpp.lo:
	${LIBTOOL} --mode=compile ${CXX} ${CXXFLAGS} -c $<

# Dependencies

Makefile.dep: ${srcdir}/Makefile.dep
	cp ${srcdir}/Makefile.dep $@

include Makefile.dep

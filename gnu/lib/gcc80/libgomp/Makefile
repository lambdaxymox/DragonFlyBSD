CFLAGS+=	-I${.CURDIR}
CFLAGS+=	-I${.OBJDIR}
.include "../Makefile.inc"

.PATH: ${GCCDIR}/libgomp
#.PATH: ${GCCDIR}/libgomp/config/bsd
.PATH: ${GCCDIR}/libgomp/config/posix

CFLAGS+=	-DHAVE_CONFIG_H
CFLAGS+=	-I${GCCDIR}/libgomp
CFLAGS+=	-I${GCCDIR}/libgomp/config/posix
CFLAGS+=	-I${GCCDIR}/libgcc
VERSION_MAP=	libgomp.ver

LIB=		gomp
SHLIB_MAJOR=	1

LDADD=		-lpthread
DPADD=		${LIBPTHREAD}

# From build/x86_64-portbld-dragonfly5.1/libgomp/Makefile
libgomp_la_SOURCES = alloc.c atomic.c barrier.c critical.c env.c \
	error.c icv.c icv-device.c iter.c iter_ull.c loop.c loop_ull.c \
	ordered.c parallel.c sections.c single.c task.c team.c work.c \
	lock.c proc.c sem.c bar.c time.c fortran.c \
	affinity.c target.c splay-tree.c libgomp-plugin.c \
	oacc-parallel.c oacc-host.c oacc-init.c oacc-mem.c \
	oacc-async.c oacc-plugin.c oacc-cuda.c priority_queue.c

SRCS=	${libgomp_la_SOURCES}

# empty sources (Everything is in the header.)
#SRCS+=	mutex.c ptrlock.c

# On gcc80 this is from Makefile.x86-64, but there is only one platform now
# Values recorded in <build-dir>/libgomp/config.log
# x86-64 platform

OMP_LOCK_25_ALIGN=	8
OMP_LOCK_25_KIND=	8
OMP_LOCK_25_SIZE=	8
OMP_LOCK_ALIGN=		8
OMP_LOCK_KIND=		8
OMP_LOCK_SIZE=		8
OMP_NEST_LOCK_25_ALIGN= 8
OMP_NEST_LOCK_25_KIND=	8
OMP_NEST_LOCK_25_SIZE=	16
OMP_NEST_LOCK_ALIGN=	8
OMP_NEST_LOCK_KIND=	8
OMP_NEST_LOCK_SIZE=	24

# tail of former Makefile.x86-64

INCSGROUPS=	hd_libdata hd_lib
hd_libdataDIR=	/usr/libdata/gcc${GCCSHORTVER}
hd_libDIR=	/usr/lib/gcc${GCCSHORTVER}

hd_libdata=	omp.h
hd_lib=		libgomp.spec

libgomp_f.h: ${GCCDIR}/libgomp/libgomp_f.h.in
	sed -e 's/@OMP_LOCK_25_ALIGN@/${OMP_LOCK_25_ALIGN}/g' \
	    -e 's/@OMP_LOCK_25_KIND@/${OMP_LOCK_25_KIND}/g' \
	    -e 's/@OMP_LOCK_25_SIZE@/${OMP_LOCK_25_SIZE}/g' \
	    -e 's/@OMP_LOCK_ALIGN@/${OMP_LOCK_ALIGN}/g' \
	    -e 's/@OMP_LOCK_KIND@/${OMP_LOCK_KIND}/g' \
	    -e 's/@OMP_LOCK_SIZE@/${OMP_LOCK_SIZE}/g' \
	    -e 's/@OMP_NEST_LOCK_25_ALIGN@/${OMP_NEST_LOCK_25_ALIGN}/g' \
	    -e 's/@OMP_NEST_LOCK_25_KIND@/${OMP_NEST_LOCK_25_KIND}/g' \
	    -e 's/@OMP_NEST_LOCK_25_SIZE@/${OMP_NEST_LOCK_25_SIZE}/g' \
	    -e 's/@OMP_NEST_LOCK_ALIGN@/${OMP_NEST_LOCK_ALIGN}/g' \
	    -e 's/@OMP_NEST_LOCK_KIND@/${OMP_NEST_LOCK_KIND}/g' \
	    -e 's/@OMP_NEST_LOCK_SIZE@/${OMP_NEST_LOCK_SIZE}/g' \
	    < ${.ALLSRC} > ${.TARGET}

omp.h: ${GCCDIR}/libgomp/omp.h.in
	sed -e 's/@OMP_LOCK_SIZE@/${OMP_LOCK_SIZE}/g' \
	    -e 's/@OMP_LOCK_ALIGN@/${OMP_LOCK_ALIGN}/g' \
	    -e 's/@OMP_NEST_LOCK_SIZE@/${OMP_NEST_LOCK_SIZE}/g' \
	    -e 's/@OMP_NEST_LOCK_ALIGN@/${OMP_NEST_LOCK_ALIGN}/g' \
	    < ${.ALLSRC} > ${.TARGET}

omp_lib.h: ${GCCDIR}/libgomp/omp_lib.h.in
	sed -e 's/@OMP_LOCK_KIND@/${OMP_LOCK_KIND}/g' \
	    -e 's/@OMP_NEST_LOCK_KIND@/${OMP_NEST_LOCK_KIND}/g' \
	    < ${.ALLSRC} > ${.TARGET}

libgomp.ver: ${GCCDIR}/libgomp/libgomp.map
	sed -e '/#/d' < ${.ALLSRC} > ${.TARGET}

libgomp.spec:
	echo "*link_gomp: -lgomp %{static: }" > ${.TARGET}

GEND_FILES=	libgomp_f.h omp.h omp_lib.h libgomp.ver libgomp.spec
CLEANFILES=	${GEND_FILES}

depend all:	${GEND_FILES}

.include <bsd.lib.mk>

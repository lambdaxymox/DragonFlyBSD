#	@(#)Makefile	8.5 (Berkeley) 3/31/94
# $FreeBSD: src/sbin/Makefile,v 1.77.2.9 2002/08/08 09:03:46 ru Exp $

SUBDIR=	adjkerntz \
	badsect \
	camcontrol \
	ccdconfig \
	clri \
	comcontrol \
	cryptdisks \
	devattr \
	devd \
	devfsctl \
	dhclient \
	dhcpcd \
	diskinfo \
	disklabel32 \
	disklabel64 \
	dmesg \
	dump \
	dumpfs \
	dumpon \
	ffsinfo \
	fsck \
	fsck_msdosfs \
	fsck_hammer2 \
	fsdb \
	fsid \
	fsirand \
	getdevpath \
	gpt \
	growfs \
	hammer \
	hammer2 \
	ifconfig \
	init \
	ip6fw \
	ipfw \
	ipfw3 \
	iscontrol \
	jscan \
	kldconfig \
	kldload \
	kldstat \
	kldunload \
	ldconfig \
	md5 \
	mount \
	mount_autofs \
	mount_cd9660 \
	mount_devfs \
	mount_dirfs \
	mount_ext2fs \
	mount_fuse \
	mount_hammer \
	mount_hammer2 \
	mount_hpfs \
	mount_msdos \
	mount_nfs \
	mount_ntfs \
	mount_null \
	mount_std \
	mount_tmpfs \
	mount_udf \
	mount_ufs \
	mountctl \
	mountd \
	natacontrol \
	natd \
	newbtconf \
	newfs \
	newfs_hammer \
	newfs_hammer2 \
	newfs_msdos \
	nfsd \
	nfsiod \
	nologin \
	nos-tun \
	nvmectl \
	ping \
	ping6 \
	quotacheck \
	rconfig \
	rcorder \
	rcrun \
	reboot \
	resolvconf \
	restore \
	route \
	routed \
	rtsol \
	savecore \
	shutdown \
	slattach \
	spppcontrol \
	startslip \
	svc \
	swapon \
	sysctl \
	tcplay \
	tunefs \
	udevd \
	umount \
	usched \
	vinum \
	vquota

.if ${MACHINE_ARCH} == "x86_64"
SUBDIR+=mount_smbfs \
	fdisk
.endif

.if exists(${.CURDIR}/${MACHINE})
SUBDIR+= ${MACHINE}
.endif
.if exists(${.CURDIR}/${MACHINE_ARCH})
SUBDIR+= ${MACHINE_ARCH}
.endif

# maximum parallelism
#
SUBDIR_ORDERED=

.include <bsd.subdir.mk>

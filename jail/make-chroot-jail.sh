#!/bin/bash

if [ "$1" == "" ]; then 
	echo "$0 <jailpath> <binary-files....>"
	exit
fi

JAIL=$1
shift

mkdir -p $JAIL/{dev,etc,lib,lib64,usr,bin}
mkdir -p $JAIL/usr/bin
mkdir -p $JAIL/usr/lib
mkdir -p $JAIL/usr/tmp
chown root.root $JAIL

mknod -m 666 $JAIL/dev/null c 1 3
mknod -m 644 $JAIL/dev/random c 1 8
mknod -m 644 $JAIL/dev/urandom c 1 9


JAIL_BIN=$JAIL/usr/bin/
JAIL_ETC=$JAIL/etc/

cp -alLvf /etc/ld.so.cache $JAIL_ETC
cp -alLvf /etc/ld.so.conf $JAIL_ETC
cp -alLvf /etc/nsswitch.conf $JAIL_ETC
cp -alLvf /etc/hosts $JAIL_ETC
cp -alLvf /etc/hosts $JAIL_ETC
cp -alLvf /etc/localtime $JAIL_ETC
cp -aLvf /etc/resolv.conf $JAIL_ETC


copy_binary()
{
	if [ ! -e $1 ]; then 
		BINARY=$(which $1)
	else 
		BINARY=$1
	fi


	d="$(dirname $BINARY)"

	[ ! -d $JAIL$d ] && mkdir -p $JAIL$d || :

	cp -alLvf $BINARY $JAIL/$BINARY

	copy_dependencies $BINARY
}

# http://www.cyberciti.biz/files/lighttpd/l2chroot.txt
copy_dependencies()
{
	FILES="$(ldd $1 | awk '{ print $3 }' |egrep -v ^'\(')"

	echo "Copying shared files/libs to $JAIL..."

	for i in $FILES
	do
		d="$(dirname $i)"

		[ ! -d $JAIL$d ] && mkdir -p $JAIL$d || :

		/bin/cp -alLvf $i $JAIL$d
	done

	sldl="$(ldd $1 | grep 'ld-linux' | awk '{ print $1}')"

	# now get sub-dir
	sldlsubdir="$(dirname $sldl)"

	if [ ! -f $JAIL$sldl ];
	then
		echo "Copying $sldl $JAIL$sldlsubdir..."
		/bin/cp -alLvrf $sldl $JAIL$sldlsubdir
	else
		:
	fi
}


while [ "$1" != "" ]; do
	copy_binary $1
	shift
done;

#!/usr/bin/env sh
set -e
PORT=$1

for c in 1 10 20 30 100 1024; do
    dd if=/dev/zero of=/tmp/zeros bs=1024 count=$c ;
    scp -o StrictHostKeyChecking=no -P ${PORT} /tmp/zeros root@localhost:/tmp/zeros1 ;
    if [ "$?" -ne 0 ] ; then
      echo "Failed to scp to port ${PORT}"
      exit 1;
    fi
    cksum=$(md5sum /tmp/zeros | cut -d  ' ' -f 1)
    cksum2=$(ssh -o StrictHostKeyChecking=no -p ${PORT} root@localhost md5sum /tmp/zeros1 | cut -d  ' ' -f 1) ;
    if [ "$?" -ne 0 ] ; then
      echo "Failed to ssh to port ${PORT}"
      exit 1;
    fi
    if [ "$cksum" = "$cksum2" ] ; then
      echo "Files match"
    else
      echo "Files do not match"
      exit 1
    fi
done

PID=$(pidof localproxy)
kill $PID

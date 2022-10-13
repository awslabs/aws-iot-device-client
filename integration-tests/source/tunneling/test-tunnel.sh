#!/usr/bin/env sh
set -e
PORT=$1

for c in 1 10 20 30 100 1024 10240; do
    dd if=/dev/zero of=/tmp/zeros bs=1024 count=$c ;
    scp -o StrictHostKeyChecking=no -P ${PORT} /tmp/zeros root@localhost:/tmp/zeros1 ;
    if [ "$?" -ne 0 ] ; then
      echo "Failed to scp to port ${PORT}"
      exit 1;
    fi
    cksum2=$(ssh -o StrictHostKeyChecking=no -p ${PORT} root@localhost cksum /tmp/zeros1) ;
    if [ "$?" -ne 0 ] ; then
      echo "Failed to ssh to port ${PORT}"
      exit 1;
    fi
    if cmp --silent -- /tmp/zeros /tmp/zeros1 ; then
      echo "Files match"
    else
      echo "Files do not match"
      exit 1
    fi
done

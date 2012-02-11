#!/bin/bash

MUSER="$1"
MPASS="$2"
MDB="$3"
MHOST="$4"
DJVUFILE="$5"

# help
if [ ! $# -ge 5 ]
then
echo "Usage: $0 {MySQL-User-Name} {MySQL-User-Password} {MySQL-Database-Name} {host-name} {DjVuFile}"
echo "Tests DjVu shape exporter. Clears the database and processes given DjVuFile"
exit 1
fi

./test_delete_tables.sh $MUSER $MPASS $MDB $MHOST
./test_create_tables.sh $MUSER $MPASS $MDB $MHOST
./exportshapes -u $MUSER -p $MPASS -d $MDB -h $MHOST $DJVUFILE

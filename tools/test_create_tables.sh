#!/bin/bash
MUSER="$1"
MPASS="$2"
MDB="$3"
MHOST="$4"

MYSQL=$(which mysql)

$MYSQL -u $MUSER --password=$MPASS -h $MHOST --verbose -D $MDB < shape.sql

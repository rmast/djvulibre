#!/bin/bash
# A shell script to delete / drop all tables from MySQL database.
# Usage: ./script user password dbnane
# Usage: ./script user password dbnane server-ip
# Usage: ./script user password dbnane mysql.nixcraft.in
# -------------------------------------------------------------------------
# Copyright (c) 2008 nixCraft project <http://www.cyberciti.biz/fb/>
# This script is licensed under GNU GPL version 2.0 or above
# -------------------------------------------------------------------------
# This script is part of nixCraft shell script collection (NSSC)
# Visit http://bash.cyberciti.biz/ for more information.
# ----------------------------------------------------------------------
# See URL for more info:
# http://www.cyberciti.biz/faq/how-do-i-empty-mysql-database/
# ---------------------------------------------------
 
MUSER="tester"
MPASS="test"
MDB="exported_shapes"
 
MHOST="localhost"
 
# Detect paths
MYSQL=$(which mysql)
AWK=$(which awk)
GREP=$(which grep)
 
# make sure we can connect to server
$MYSQL -u $MUSER -p$MPASS -h $MHOST -e "use $MDB"  &>/dev/null
if [ $? -ne 0 ]
then
	echo "Error - Cannot connect to mysql server using given username, password or database does not exits!"
	exit 2
fi
 
TABLES=$($MYSQL -u $MUSER -p$MPASS -h $MHOST $MDB -e 'show tables' | $AWK '{ print $1}' | $GREP -v '^Tables' )
 
# make sure tables exits
if [ "$TABLES" == "" ]
then
	echo "Error - No table found in $MDB database!"
	exit 3
fi
 
# let us do it
for t in $TABLES
do
	echo "Deleting $t table from $MDB database..."
	$MYSQL -u $MUSER -p$MPASS -h $MHOST $MDB -e "drop table $t"
done

$MYSQL -u $MUSER --password=$MPASS --verbose -D $MDB < shape.sql


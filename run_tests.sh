#!/bin/bash

if [[ ! -f relocate.c ]];
then
     echo "Failed. The file reloacate.c does not exist"	
     exit -1
fi

tail -n 33 relocate.c > /tmp/tailcheck.txt
err=`diff /tmp/tailcheck.txt testcases/testcase1.txt`
rm -f /tmp/tailcheck.txt

if [[ "$err" != "" ]];
then
    echo "Did not follow the instructions in Code editing! Will not be evaulated"
    echo "###### Diff ######"
    echo $err
    exit -1
fi


do_test() {
    ./relocate > e_tmp/output.$1 2>&1
    
    if [[ $? != 0 ]];
    then
        echo "Test case $1 failed"
	return
    fi
    
    err=`grep "Segmentation fault" e_tmp/output.$1`
    if [[ "$err" != "" ]];
    then
        echo "Test case $1 failed"
	return
    fi
    
    maddr=`grep Main e_tmp/output.$1 | cut -d ":" -f2`
    RIP=`grep RIP e_tmp/output.$1 | cut -d ":" -f2`
    d=`echo "$RIP - $maddr" | bc`

    if [[ $d -le 0 || $d -ge 8192 ]];
    then
        echo "Test case $1 failed"
    else
        echo "Test case $1 passed"
    fi
}

rm -f relocate

##### Dynamically Linked #####
gcc relocate.c -o relocate

if [[ ! -f relocate ]];
then
     echo "Failed. Compilation error"	
     exit -1
fi

mkdir e_tmp
rm -rf output
mkdir output

do_test 1

##### Statically Linked #####

gcc -static relocate.c -o relocate
do_test 2

cp e_tmp/* output/
rm -rf e_tmp

rm -f relocate

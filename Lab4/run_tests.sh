#!/bin/bash

if [[ ! -f memMagic.c ]];
then
     echo "Failed. The file count.c does not exist"	
     exit -1
fi

rm -f memMagic

na=`grep "malloc" memMagic.c`
if [[ "$na" != "" ]];
then
     echo "Your solution uses some construct that is not allowed culprit --> $na"	
     exit -1

fi

na=`grep "free(" memMagic.c`
if [[ "$na" != "" ]];
then
     echo "Your solution uses some construct that is not allowed culprit --> $na"	
     exit -1

fi

na=`grep "realloc" memMagic.c`
if [[ "$na" != "" ]];
then
     echo "Your solution uses some construct that is not allowed culprit --> $na"	
     exit -1

fi

na=`grep "mmap" memMagic.c`
if [[ "$na" == "" ]];
then
     echo "Your solution doesnot use some construct that is expected --> mmap"	
     exit -1

fi

na=`grep "munmap" memMagic.c`
if [[ "$na" == "" ]];
then
     echo "Your solution doesnot use some construct that is expected --> munmap"	
     exit -1

fi

na=`grep "mremap" memMagic.c`
if [[ "$na" == "" ]];
then
     echo "Your solution doesnot use some construct that is expected --> mremap"	
     exit -1

fi


gcc memMagic.c -o memMagic

if [[ ! -f memMagic ]];
then
     echo "Failed. Compilation error"	
     exit -1
fi

output=$(./memMagic)
if [[ "$output" != "Program surgery successful!" ]]
then
     echo "Failed. Program produced unexpected output: ${output}"	
     exit -1
fi

echo "Test passed"


#!/bin/bash 

DISALLOWED="fopen|fclose|fread|fork|system|exec"


if [[ ! -f memspan.c ]];
then
     echo "Failed. The file cprime.c does not exist"	
     exit -1
fi

if grep -Eq "$DISALLOWED" memspan.c; 
then
    echo "Failed. Using restricted APIs?"
    exit 1
fi

make clean > /dev/null 2>&1
make > /dev/null 2>&1

if [[ ! -f memspan ]];
then
     echo "Failed. Compilation error"	
     exit -1
fi

for tc in 1 2 3 4 5 6 7 8 9 10
do
      ./memspan $tc 
      if [[ $? -eq 0 ]];
      then
	      echo "Test case $tc passed"
      else
	      echo "Test case $tc failed"
      fi
done

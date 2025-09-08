#!/bin/bash

for impl in ref light
do
  echo Generating $impl answers
  for i in build/bin/*_${impl}
  do
      echo Generating KATs for $i
      ./$i
  done
  kat_dir=../../../KAT/$impl
  if [[ ! -e $kat_dir ]]
  then
    mkdir $kat_dir
  fi
  mv *.req $kat_dir
  mv *.rsp $kat_dir
done


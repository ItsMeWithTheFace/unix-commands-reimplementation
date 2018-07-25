#!/bin/bash

if [ $1 = 'dl' ]
then
  if [ -z $2 ]; then 
    curl -o ./images/emptydisk.img https://www.teach.cs.toronto.edu//~csc369h/summer/tutorials/w10/images/emptydisk.img
    curl -o ./images/onefile.img https://www.teach.cs.toronto.edu//~csc369h/summer/tutorials/w10/images/onefile.img
    curl -o ./images/deletedfile.img https://www.teach.cs.toronto.edu//~csc369h/summer/tutorials/w10/images/deletedfile.img
    curl -o ./images/onedirectory.img https://www.teach.cs.toronto.edu//~csc369h/summer/tutorials/w10/images/onedirectory.img
    curl -o ./images/hardlink.img https://www.teach.cs.toronto.edu//~csc369h/summer/tutorials/w10/images/hardlink.img
    curl -o ./images/deleteddirectory.img https://www.teach.cs.toronto.edu//~csc369h/summer/tutorials/w10/images/deleteddirectory.img
    curl -o ./images/twolevel.img https://www.teach.cs.toronto.edu//~csc369h/summer/tutorials/w10/images/twolevel.img
    curl -o ./images/largefile.img https://www.teach.cs.toronto.edu//~csc369h/summer/tutorials/w10/images/largefile.img
  else
    curl -o ./images/${2}.img https://www.teach.cs.toronto.edu//~csc369h/summer/tutorials/w10/images/${2}.img
  fi
elif [ $1 = 'clean' ]; then
  rm  ./images/*
fi




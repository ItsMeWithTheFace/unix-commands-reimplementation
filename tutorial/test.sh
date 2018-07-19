gcc readimage.c -Wall
echo "emptydisk"
./a.out ../images/emptydisk.img
echo "twolevel"
./a.out ../images/twolevel.img
echo "onefile"
./a.out ../images/onefile.img


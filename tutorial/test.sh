gcc readimage.c -Wall

echo "========================"
echo "emptydisk"
echo "========================"
./a.out ../images/emptydisk.img

echo "========================"
echo "onefile"
echo "========================"
./a.out ../images/onefile.img

echo "========================"
echo "deletedfile"
echo "========================"
./a.out ../images/deletedfile.img

echo "========================"
echo "onedirectory"
echo "========================"
./a.out ../images/onedirectory.img

echo "========================"
echo "hardlink"
echo "========================"
./a.out ../images/hardlink.img

echo "========================"
echo "deleteddirectory"
echo "========================"
./a.out ../images/deleteddirectory.img

echo "========================"
echo "twolevel"
echo "========================"
./a.out ../images/twolevel.img

echo "========================"
echo "largefile"
echo "========================"
./a.out ../images/largefile.img

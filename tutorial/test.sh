gcc readimage.c -Wall

image1 = $1
image2 = $2
image3 = $3
image4 = $4
image5 = $5
image6 = $6

if [ -n image1  ]
then
    echo "========================"
    echo $1
    echo "========================"
    ./a.out ../images/$1
else
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
  fi

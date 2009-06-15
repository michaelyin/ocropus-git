if [ $# -ne 4 ]
then
  echo "Usage: `basename $0` <google-book-dir> <model-for-alignment> <output-dir-for-images> <new-model>"
  exit -1
fi

element="ocr_cinfo"
min_len=2
max_len=20
cmodel=$2
./hocr-extract-g1000 $1/hOCR.html "$1/Image_????.JPEG" "$3/%04d/%04d"
ocropus lines2fsts $3
ocropus align $3
ocropus trainseg $4 $3


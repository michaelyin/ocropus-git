element="ocr_line"
min_len=10
max_len=100
cmodel=$2
./hocr-extract-g1000 $1/hOCR.html "$1/Image_????.JPEG" "$3/%04d/%04d"
ocropus lines2fsts $3
ocropus align $3
ocropus trainseg $4 $3


if [ $# -ne 4 ]
then
  echo "Usage: `basename $0` <jstor-data-dir> <model-for-alignment> <output-dir-for-images> <new-model>"
  exit -1
fi

cmodel=$2
./jstor.py "$1/ocrs/*.xml" "$1/pages/*.png" "$3/%04d/%04d" > $3/jstor-extract.log
ocropus lines2fsts $3
ocropus align $3
for i in $3/???? ; do rename 's/\.cseg\.png$/\.cseg\.gt\.png/g'  $i/????.cseg.png ; done
ocropus trainseg $4 $3


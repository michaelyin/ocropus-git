CXX=g++
CXXFLAGS= \
	-I/usr/local/include/ \
	-I/usr/local/include/ocropus \
	-I/usr/local/include/colib/ \
	-I/usr/local/include/iulib/ \
	-fopenmp

LDFLAGS=-L/usr/local/lib -L/usr/local/lib/ -llinerec -locropus -liulib -lpng -ljpeg -ltiff -lSDL -lSDL_gfx -ltesseract_full -lpthread -lgsl -lgslcblas

all: pageseg

pageseg: pageseg.cc
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@


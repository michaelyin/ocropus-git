CXX=g++
CXXFLAGS= \
	-I/usr/local/include/ \
	-I/usr/local/include/ocropus \
	-I/usr/local/include/colib/ \
	-I/usr/local/include/iulib/ \
	-fopenmp

LDFLAGS=-L/usr/local/lib/ -locropus -liulib -lpng -ljpeg -ltiff -lSDL -lSDL_gfx -ltesseract_full -lpthread -lgsl -lgslcblas

all: cseg2mnist

cseg2mnist: cseg2mnist.cc mnist.h
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@

clean:
	rm cseg2mnist

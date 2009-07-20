all: ocrothresh ocrothresh.so

ocrothresh: ocrothresh.cc
	g++ -g -o ocrothresh ocrothresh.cc -locropus -liulib -ljpeg -lpng -lgif -ltiff -fopenmp -llept -lSDL -lSDL_gfx -lgsl -lblas

ocrothresh.so: ocrothresh.cc
	g++ -fPIC -shared -g -o ocrothresh.so ocrothresh.cc -locropus -liulib -ljpeg -lpng -lgif -ltiff -fopenmp -llept -lSDL -lSDL_gfx -lgsl -lblas

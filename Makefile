CXX=g++
all:graphics
    
graphics: graphics.o
	${CXX} graphics.o -o graphics   

graphics.o: graphics.cpp
	${CXX} -c graphics.cpp -o graphics.o

clean:
	rm graphics graphics.o

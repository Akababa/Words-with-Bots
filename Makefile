CXX = g++
CXXFLAGS = -std=c++14 -Wall -Wno-sign-compare -MMD
OBJECTS = wwf.o
DEPENDS = ${OBJECTS:.o=.d}
EXEC = a 

${EXEC}: ${OBJECTS}
	${CXX} ${OBJECTS} -o ${EXEC}

-include ${DEPENDS} # copies files x.d, y.d, z.d (if exists)
    
.PHONY: clean # not a file name
clean: # remove files that can be regenerated
	rm -rf ${DEPENDS} ${OBJECTS} ${EXEC}
	
all: a


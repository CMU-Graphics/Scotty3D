
CPP=g++ -Wall -Werror -g -std=c++17

all : sejp-test example

sejp-test : sejp-test.o sejp.o
	$(CPP) -o '$@' $^

example : example.o sejp.o
	$(CPP) -o '$@' $^

sejp-test.o : sejp-test.cpp sejp.hpp
	$(CPP) -c -o '$@' '$<'

example.o : example.cpp sejp.hpp
	$(CPP) -c -o '$@' '$<'

sejp.o : sejp.cpp sejp.hpp
	$(CPP) -c -o '$@' '$<'

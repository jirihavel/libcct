PKGS:=opencv argtable2

#FLAGS:=-std=c++11 -march=native -O0 -Wall -Wextra -Iinclude -ggdb -DBOOST_ALL_DYN_LINK
FLAGS:=-std=c++11 -march=native -O3 -Wall -Wextra -Iinclude -ggdb -DNDEBUG -DBOOST_DISABLE_ASSERTS -DBOOST_ALL_DYN_LINK
FLAGS+=$(shell pkg-config --cflags $(PKGS))

LIBS:=-O -lboost_chrono -lboost_thread -lboost_system
LIBS+=$(shell pkg-config --libs $(PKGS))

.PHONY:all

all:bin/imgtree-struct.exe bin/imgtree-array.exe bin/imgtree-najman.exe

bin/%.exe:obj/%.cpp.o
	$(CXX) -o $@ $^ $(LIBS)
#	objcopy --only-keep-debug $@ $@.debug
#	strip -g $@
#	objcopy --add-gnu-debuglink=$@.debug $@ 

obj/%.cpp.o: src/%.cpp
	$(CXX) -MMD -c $(FLAGS) -o $@ $<

-include $(wildcard obj/*.d)

# vim: set ft=make:
################################################################################
# Initialize the build platform
################################################################################
# Copy&Paste this to any new Makefile, modify <makedir>
# <makedir> is path from Makefile to platform.make
makedir:=make
# <makefile> is filename of this makefile
makefile:=$(lastword $(MAKEFILE_LIST))
# include the platform header (and config.make)
include $(dir $(makefile))$(if $(makedir),$(makedir)/)platform.make
# - now we can use "$(MAKEDIR)/..." and "$(srcdir)/..."
# Copy&Paste end

# -- Settings --

LANG:=cxx14

FLAGS:=-Wall -Wextra

ifneq ($(DEBUG),)
 SUFFIX:=-d
 CONFIG:=debug
 FLAGS+=-O0 -ggdb -DBOOST_DISABLE_ASSERTS
else
 FLAGS+=-O2 -g0 -DNDEBUG -DBOOST_DISABLE_ASSERTS
endif

# -- Unit tests --

WANT_TARGET:=

PKGS:=boost_unit_test_framework opencv

NAME:=check-libcct
SRCS:=$(wildcard $(SRCDIR)/check/libcct/*.cpp)
include $(MAKEDIR)/bin.make
check-libcct:$(BIN)
	$<

NAME:=check-libcct-array
SRCS:=$(wildcard $(SRCDIR)/check/libcct_array/*.cpp)
include $(MAKEDIR)/bin.make
check-libcct-array:$(BIN) check-libcct
	$<

PKGS+=boost_system

NAME:=check-libcct-struct
SRCS:=$(wildcard $(SRCDIR)/check/libcct_struct/*.cpp)
include $(MAKEDIR)/bin.make
check-libcct-struct:$(BIN) check-libcct
	$<

check:check-libcct-array check-libcct-struct
	@

# -- Tests --

WANT_TARGET:=1

DATA=data/big
IMAGES:=$(wildcard $(DATA)/*.jpg) $(wildcard $(DATA)/*.tif)

PKGS:=boost opencv

NAME:=test-extract
SRCS:=$(SRCDIR)/test/extract.cpp
include $(MAKEDIR)/bin.make

.PHONY:extract
extract:bin/test-extract$(SUFFIX)$(BINEXT)
	$< $(IMAGES)

NAME:=test-union-find
SRCS:=$(SRCDIR)/test/union_find.cpp
include $(MAKEDIR)/bin.make

.PHONY:union-find
union-find:bin/test-union-find$(SUFFIX)$(BINEXT)
	$< $(IMAGES)

.PHONY:imgtree
imgtree:imgtree-array imgtree-struct

PKGS:=argtable2 boost_chrono opencv

NAME:=imgtree-najman
SRCS:=$(SRCDIR)/imgtree-najman.cpp
include $(MAKEDIR)/bin.make

NAME:=imgtree-array
SRCS:=$(SRCDIR)/imgtree-array.cpp
include $(MAKEDIR)/bin.make

NAME:=imgtree-struct
SRCS:=$(SRCDIR)/imgtree-struct.cpp
include $(MAKEDIR)/bin.make

N=10
#D=0
PREFIX=bsds
OUT=data/out
MACHINE=i7
P=

test-%:bin/imgtree-%$(SUFFIX)$(BINEXT)
	$< $(IMAGES) -n$N $P > data/$*-$(PREFIX)-$(MACHINE)-d0.csv

# -- Binaries --

NAME:=array
PKGS:=boost eigen3 opencv
SRCS:=$(SRCDIR)/array.cpp
include $(MAKEDIR)/bin.make

NAME:=struct
PKGS:=argtable2 boost_system opencv
SRCS:=$(SRCDIR)/struct.cpp
include $(MAKEDIR)/bin.make

DEPS:=array struct
include $(MAKEDIR)/rules.make

#PKGS:=opencv argtable2 boost_asio boost_thread eigen3
#SRCS:=

#FLAGS:=-std=c++11 -march=native -O0 -Wall -Wextra -Iinclude -ggdb -DBOOST_ALL_DYN_LINK
#FLAGS:=-std=c++14 -march=native -O2 -Wall -Wextra -Iinclude -ggdb -DNDEBUG -DBOOST_DISABLE_ASSERTS
#FLAGS:=-std=c++14 -march=native -Wall -Wextra -Iinclude -ggdb
#FLAGS+=$(shell pkg-config --cflags $(PKGS))

#LIBS:=-std=c++14 -march=native
#LIBS+=$(shell pkg-config --libs $(PKGS))

#.PHONY:all

#.SECONDARY:obj/%.o

#all:bin/imgtree-array.exe bin/imgtree-najman.exe bin/imgtree-struct.exe bin/imgtree-parallel.exe

#array:bin/array.exe

#bin/%.exe:obj/%.cpp.o
#	$(CXX) -o $@ $^ $(LIBS)
#	objcopy --only-keep-debug $@ $@.debug
#	strip -g $@
#	objcopy --add-gnu-debuglink=$@.debug $@ 

#obj/%.cpp.o: src/%.cpp
#	$(CXX) -MMD -c $(FLAGS) -o $@ $<

#-include $(wildcard obj/*.d)

# -*- mode: makefile -*-
# 
# Makefile for compiling 'tmp102d' on Raspberry Pi. 
#
# Tue Feb 25 21:25:42 CET 2014
# Edit: 
# Jaakko Koivuniemi

CXX           = gcc
CXXFLAGS      = -g -O -Wall -I/usr/include/mysql/ 
LD            = gcc
LDFLAGS       = -L/usr/lib/mysql -lmysqlclient -lsqlite3 -O

%.o : %.c
	$(CXX) $(CXXFLAGS) -c $<

all: tmp102d 

tmp102d: tmp102d.o
	$(LD) $(LDFLAGS) $^ -o $@

clean:
	rm -f *.o


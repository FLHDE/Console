# Makefile for console.
# Jason Hood, 20 October, 2009 (updated 6 October, 2013).

FLEXE = \games\Freelancer\EXE
CPPFLAGS = /nologo /W3 /GX /O2 /MD /LD

console.dll: console.obj binaryrdl.obj console.res common.lib dalib.lib server.lib
	cl $(CPPFLAGS) console.obj binaryrdl.obj console.res user32.lib /link /filealign:512 /base:0x6110000

common.lib: common.def
	lib /nologo /machine:ix86 /def:common.def /name:COMMON

dalib.lib: dalib.def
	lib /nologo /machine:ix86 /def:dalib.def /name:DALIB

server.lib: server.def
	lib /nologo /machine:ix86 /def:server.def /name:SERVER

console.obj: console.cpp common.h dalib.h server.h binaryrdl.h
binaryrdl.obj: binaryrdl.cpp binaryrdl.h

install: console.dll
	copy console.dll $(FLEXE)

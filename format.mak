HEADER = $(shell for file in `find . -name *.h`;do echo $$file; done)
HEADER = $(shell for file in `find . -name *.hpp`;do echo $$file; done)
C_SOURCE = $(shell for file in `find . -name *.c`;do echo $$file; done)
CPP_SOURCE = $(shell for file in `find . -name *.cpp`;do echo $$file; done)
SOURCES = $(HEADER) $(C_SOURCE) $(CPP_SOURCE) $(OBJC_SOURCE)

all:
	for SOURCE in $(SOURCES); do sh format.sh $$SOURCE; done


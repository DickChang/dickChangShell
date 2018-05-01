#Compiler
#CC:=gcc
CC:=g++

#Target Executable Name
EXECUTABLE := main

#Directories
SRCDIR 		:= src
INCDIR		:= inc
BUILDDIR 	:= obj
TARGETDIR 	:= bin

#File Extensions
SRCEXT := cpp
HDREXT := h
OBJEXT := o

#Flags
CFLAGS := -ansi -pedantic -Wall -O3
CFLAGS := $(CFLAGS) -Iinc
CFLAGS := $(CFLAGS) -std=c++14
#CFLAGS := $(CFLAGS) -ggdb

#Libraries
LIBS :=
# LIBS := -lutil
#LIBS := $(LIBS) -lstdc++   # needed for std::chrono
#LIBS := $(LIBS) -lpthread  # lpthread is needed for std::thread

#Auto find the source files and object file paths
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))


#Default build rule
all: directories $(EXECUTABLE)

#Create the folder for the object files and executable
directories:
	@mkdir -p $(BUILDDIR)
	@mkdir -p $(TARGETDIR)


#Compile
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGETDIR)/$@ $^ $(LIBS)


#Make the object files from the source files
$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	$(CC) $(CFLAGS) -c -o $@ $<

#Delete the objects and executables
clean:
	@rm -rf $(BUILDDIR) $(TARGETDIR)

fresh: clean all

.PHONY: all directories clean fresh


#************************************************************
#	START OF PARAMETERS AREA
#************************************************************
CC		:= gcc
CFLAGS	:= -Wall -Wextra -g -D_POSIX_C_SOURCE=200112L -pthread
LIBRARIES	:=

#Folders
BIN		:= bin
SRC		:= src
INCLUDE	:= include
LIB		:= lib
OBJ		:= obj

#List of c file with main function inside
#**IMPORTANT**
#Remember to ADD a dedicated compilation command
#in "Build executables" for each EXE
EXE_1	:= $(BIN)/main
EXE_2	:= $(BIN)/test_squeue
EXES	:= $(EXE_1) $(EXE_2)
#List of object files needed by each program
OBJECTS_1	:= $(OBJ)/main.o #$(OBJ)/utilities.o 
OBJECTS_2	:= $(OBJ)/Test/Test_SQueue.o  $(OBJ)/SQueue.o  $(OBJ)/utilities.o 

#************************************************************
#	END OF PARAMETERS AREA
#************************************************************

#Get all subfolders
SOURCEDIRS	:= $(shell find $(SRC) -type d)
INCLUDEDIRS	:= $(shell find $(INCLUDE) -type d)
LIBDIRS		:= $(shell find $(LIB) -type d)

#List of all sources contained in SRC folder and subfloders
SRCS    := $(shell find $(SRC) -type f -name *.c)
#List of all objects files related to sources files
OBJS    := $(patsubst $(SRC)/%,$(OBJ)/%,$(SRCS:.c=.o))

CINCLUDES	:= $(patsubst %,-I%, $(INCLUDEDIRS:%/=%))
CLIBS		:= $(patsubst %,-L%, $(LIBDIRS:%/=%))

.PHONY: all dir clean clean_all doc docker_build docker_run

all: $(EXES)

#Build objects files to link
$(OBJ)/%.o: $(SRC)/%.c $(INCLUDEDIRS)/*
	$(CC) -c $(CFLAGS) $(CINCLUDES) $(CLIBS) $< -o $@ $(LIBRARIES)

#Build executables ******* EACH EXE MUST HAVE ITS COMPILE COMMAND
$(EXE_1):	$(OBJECTS_1)
	$(CC) $(CFLAGS) $(CINCLUDES) $(CLIBS) $^ -o $@ $(LIBRARIES)

$(EXE_2):	$(OBJECTS_2)
	$(CC) $(CFLAGS) $(CINCLUDES) $(CLIBS) $^ -o $@ $(LIBRARIES)

#Create BIN and OBJ folder if they are missing.
#OBJ folder will have the same structure of SRC folder.
dir: $(OBJ) $(BIN)
$(OBJ):
	mkdir -p $(patsubst $(SRC)%,$(OBJ)%,$(SOURCEDIRS))
$(BIN):
	mkdir -p $@

doc:
	doxygen Doxyfile

clean:
	-rm -r $(BIN)/*
	-rm -r $(OBJ)/*

#Build docker image for the project
docker_build:
	docker build -t market:0.1 .

docker_run:
	docker run --rm -ti -v $(shell pwd):/usr/src market:0.1

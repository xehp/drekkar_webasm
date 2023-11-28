# Configureations.
################################


ifeq ($(OS),Windows_NT)
  TARGET=WIN32
endif


# Compiler to use.
CC=gcc

# To cross compile for windows, use mingw. Install mingw and wine. 
# sudo apt-get install gcc-mingw-w64-x86-64 wine-stable
# /usr/x86_64-w64-mingw32/lib/
# Then uncomment the line below and do make (on windows mingw32make or something).
#CC=/usr/bin/x86_64-w64-mingw32-gcc 
#TARGET=WIN32

# To compile on Windows, also gcc, but mingw make will have another name.
# Use this make: mingw32-make.exe -j4
# Run mingw make from within git-bash. 

# Dont think we use threads any more. Cut if we do the lib is also needed.
# LIBS += pthread

# if math.h is used its lib is also needed.
# LIBS += m

# Uncomment if release build.
DEBUG= true

# Flags to be passed on to the C compiler and/or linker.
ifneq ($(TARGET),WIN32)
CFLAGS= -Werror -Wall -Wno-format-truncation -Wno-error=unused-function -Wno-error=unused-but-set-variable -Wno-error=unused-variable -I. -std=gnu11 
LDFLAGS= -lm
else
#CFLAGS= -I. -std=c99 -m32 -DEWOULDBLOCK=WSAEWOULDBLOCK
CFLAGS= -I. -std=gnu11 -DEWOULDBLOCK=WSAEWOULDBLOCK
LDFLAGS= -l ws2_32
endif

# optionally add -DUSE_THREADS to CFLAGS below.
# Only works in linux version, might reduce CPU usage.
#CFLAGS+= -DUSE_THREADS

# Put anything here for it to be configured by pkg-config.
PKG_LIBS=

# Folders:
OBJDIR=obj
SRCDIR=src

################################

# Made by: Alexander Bjorkman 2016
# Version: 0.1.1

ifdef DEBUG
	CFLAGS+= -O0 -g
else
	CFLAGS+= -O3
endif

ifdef PKG_LIBS
	PKG_OUTPUT=$(shell pkg-config --cflags --libs $(PKG_LIBS))
	CFLAGS+=$(PKG_OUTPUT)
	LDFLAGS+=$(PKG_OUTPUT)
endif


# Get the target name
CASEEXEC=$(shell basename $(CURDIR))
ifneq ($(TARGET),WIN32)
EXEC=$(shell echo $(CASEEXEC) | tr A-Z a-z)
else
EXEC=$(shell echo $(CASEEXEC) | tr A-Z a-z).exe
endif

# Get sources to compile.
SOURCES=$(shell find $(SRCDIR) -name "*.c")

# Find the sub-folders of src
SRCDIRS=$(shell find $(SRCDIR) -type d)

# Add a prefix to the folders.
DIRECTORIES=$(addprefix $(OBJDIR)/, $(SRCDIRS))

# Get their names.
UNOBJECTS=$(SOURCES:.c=.o)

# Append objdir to the object names.
OBJECTS=$(addprefix $(OBJDIR)/, $(UNOBJECTS))

# Include the libs...
LDFLAGS+=$(addprefix -l,$(LIBS))

# Creates a list of .d files we could include.
DEPS := $(OBJECTS:.o=.d)

# The goal is to get the executable.
all: $(EXEC)

# To clean we simply remove everything that is generated.
clean:
	rm -r $(OBJDIR)
	rm $(EXEC) 


# Self explanatory.
help:
	@echo "Type: 'make all'"
	@echo "	To build everthing."
	@echo "Type: 'make clean'"
	@echo "	To clean after build"
	@echo "Type: 'make help'"
	@echo "	For this help message."

# The executable needs the object files.
# 	To create it we execute the linker
$(EXEC): $(OBJDIR)/$(SRCDIR) $(OBJECTS) $(DIRECTORIES)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

# Automatic dependency graph generation
-include $(OBJECTS:.o=.d)

# Here we compile a object file using it's c file partner.
$(OBJDIR)/%.o: %.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c -MMD -MT $@ -MF $(patsubst %.o,%.d,$@) $< -o $@

# Make sure the sub-folders in src/ exists in obj/.
$(DIRECTORIES):
	@echo "Creating directories!"
	mkdir -p $(OBJDIR)
	mkdir -p $@

# Explanation
# $@ is what you are trying to create.
# $< Is the first requirement me thinks.
# what-to-create: dependencies
#	[tab] system commands
#
# SOMETHING=$(SOURCE:.a=.b)
# In this example SOMETHING becomes everything SOURCE is
# but .a is replaced by .b
#
# EVERYTHING=$(wildcard *.xyz)
# Here EVERYTHING becomes all list of files matching
# the regex. In this case the regex is "*.xyz"
#
# Note that "all" rules is the default rule.
#
# Any system command with a "@" prefix is quiet.
#
# Note that the prefix "-" in "-include" allows the
# system command to fail. In other words not fin the .d file.

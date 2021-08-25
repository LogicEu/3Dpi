# 3Dpi makefile

STD=-std=c99
WFLAGS=-Wall -Wextra
OPT=-O2
IDIR=-I. -Iinclude
SLIBS=utopia fract mass glee gleex imgtool
DLIBS=glfw z png jpeg freetype
CC=gcc
NAME=3Dpi
SRC=*.c

IDIR += $(patsubst %,-I%/,$(SLIBS))
CFLAGS=$(STD) $(WFLAGS) $(OPT) $(IDIR)
OS=$(shell uname -s)

ifeq ($(OS),Darwin)
	OSFLAGS=-framework OpenGL -mmacos-version-min=10.9
else
#WLFLAG=-Wl,--whole-archive
#WRFLAG=-Wl,--no-whole-archive
	OSFLAGS=-lm -lGL -lGLEW
endif

LDIR=lib
LSTATIC=$(patsubst %,lib%.a,$(SLIBS))
LPATHS=$(patsubst %,$(LDIR)/%,$(LSTATIC))
LFLAGS=$(patsubst %,-L%,$(LDIR))
LFLAGS += $(WLFLAG)
LFLAGS += $(patsubst %,-l%,$(SLIBS))
LFLAGS += $(WRFLAG)
LFLAGS += $(patsubst %,-l%,$(DLIBS))
LFLAGS += $(OSFLAGS)

$(NAME): $(LPATHS) $(SRC)
	$(CC) -o $@ $(SRC) $(CFLAGS) $(LFLAGS)

$(LDIR)/$(LDIR)%.a: $(LDIR)%.a $(LDIR)
	mv $< $(LDIR)/

$(LDIR): 
	mkdir $@

$(LDIR)%.a: %
	cd $^ && make && mv $@ ../

clean:
	rm -r $(LDIR) && rm $(NAME)

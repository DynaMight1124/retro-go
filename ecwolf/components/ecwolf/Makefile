PRGNAME     = ecwolf

# define regarding OS, which compiler to use
EXESUFFIX = 
TOOLCHAIN = 
CC          = gcc
CCP         = g++
LD          = gcc

SDL_CFLAGS  := $(shell /usr/bin/sdl-config --cflags)
SDL_LDFLAGS  := $(shell /usr/bin/sdl-config --libs)

# change compilation / linking flag options
F_OPTS		= -DHOME_SUPPORT -DNO_GTK -DUSE_GPL -DGCW0 -DNONET
CC_OPTS		= -O0 -g3 $(F_OPTS) $(SDL_CFLAGS) -Isrc/resourcefiles -I.
CFLAGS      = -Iinclude -I./bzip2 -I./deps/gdtoa -Isrc/g_shared -I./jpeg-6b -I./src -I./src/dosbox -I./src/g_blake -I./src/g_shareds -I./src/g_wolf -I./deps/lzma/C -I./src/r_2d -I./src/r_data -I./src/textures -I./src/sfmt -I./src/thingdef
CFLAGS		+= -I$(SDL_INCLUDE) $(CC_OPTS) -Ideps/bzip2 -Ideps/gdtoa -Dstrnicmp=strncasecmp -Dstricmp=strcasecmp -DUNDER_CE -DUSE_GPL -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp
CXXFLAGS	= $(CFLAGS) -fno-rtti 
LDFLAGS     = $(SDL_LDFLAGS) -lSDL_image -lSDL_mixer -lSDL_net -ljpeg -lpng -lm -lz -lstdc++

# Files to be compiled
SRCDIR    	= ./bzip2 ./src ./src/resourcefiles ./src/dosbox ./src/g_blake ./src/g_shared ./jpeg-6b
SRCDIR    	+= ./src/g_shareds ./src/g_wolf ./src/r_2d ./src/r_data ./src/textures ./src/sfmt ./src/thingdef ./deps/lzma/C ./deps/bzip2
SRCDIR    	+= ./textscreen  ./deps/gdtoa
VPATH     	= $(SRCDIR)
SRC_C   	= $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.c))
SRC_CP   	= $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.cpp))
OBJ_C   	= $(notdir $(patsubst %.c, %.o, $(SRC_C)))
OBJ_CP   	= $(notdir $(patsubst %.cpp, %.o, $(SRC_CP)))
OBJS     	= $(OBJ_C) $(OBJ_CP)

# Rules to make executable
$(PRGNAME)$(EXESUFFIX): $(OBJS)  
	$(LD) -o $(PRGNAME)$(EXESUFFIX) $^ $(LDFLAGS)

$(OBJ_C) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_CP) : %.o : %.cpp
	$(CCP) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(PRGNAME)$(EXESUFFIX) *.o

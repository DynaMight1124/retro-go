STATIC_LINKING := 0

CORE_DIR     := .

CFLAGS       :=
CXXFLAGS     :=

# platform
ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -s),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -s)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -s)),)
   platform = osx
else ifneq ($(findstring win,$(shell uname -s)),)
   platform = win
endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
ifeq ($(shell uname -p),arm)
	arch = arm
endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

TARGET_NAME := vemulator
LIBM		= -lm

ifeq ($(STATIC_LINKING), 1)
EXT := a
endif

ifneq ($(findstring unix,$(platform)),)
	EXT ?= so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
else ifeq ($(platform), linux-portable)
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC -nostdlib
   SHARED := -shared -Wl,--version-script=link.T
	LIBM :=
else ifeq ($(platform), osx)
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib

   ifeq ($(CROSS_COMPILE),1)
		TARGET_RULE   = -target $(LIBRETRO_APPLE_PLATFORM) -isysroot $(LIBRETRO_APPLE_ISYSROOT)
		CFLAGS   += $(TARGET_RULE)
		CPPFLAGS += $(TARGET_RULE)
		CXXFLAGS += $(TARGET_RULE)
		LDFLAGS  += $(TARGET_RULE)
   endif

   CFLAGS += $(ARCHFLAGS)
   LFLAGS += $(ARCHFLAGS)

else ifneq (,$(findstring ios,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
        MINVERSION :=

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif

   ifeq ($(platform),ios-arm64)
      CC = cc -arch arm64 -isysroot $(IOSSDK)
      CXX = clang++ -arch arm64 -isysroot $(IOSSDK)
   else
      CC = cc -arch armv7 -isysroot $(IOSSDK)
      CXX = clang++ -arch armv7 -isysroot $(IOSSDK)
   endif
ifeq ($(platform),$(filter $(platform),ios9 ios-arm64))
	MINVERSION = -miphoneos-version-min=8.0
else
	MINVERSION = -miphoneos-version-min=5.0
endif
        CFLAGS += $(MINVERSION)
        CXXFLAGS += $(MINVERSION)
        LDFLAGS += $(MINVERSION)

else ifeq ($(platform), tvos-arm64)
   TARGET := $(TARGET_NAME)_libretro_tvos.dylib
   fpic := -fPIC
   SHARED := -dynamiclib

   ifeq ($(IOSSDK),)
      IOSSDK := $(shell xcodebuild -version -sdk appletvos Path)
   endif

   CFLAGS   += -DIOS
   CXXFLAGS += -DIOS

   CC = cc -arch arm64 -isysroot $(IOSSDK)
   CXX = clang++ -arch arm64 -isysroot $(IOSSDK)

else ifneq (,$(findstring qnx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_emscripten.bc
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
else ifeq ($(platform), vita)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC ?= arm-vita-eabi-gcc
   CXX ?= arm-vita-eabi-g++
   AR ?= arm-vita-eabi-ar
   CFLAGS += -Wl,-q -Wall
   CXXFLAGS += -Wl,-q -Wall
   STATIC_LINKING = 1
else ifeq ($(platform), miyoo)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=link.T  -Wl,--no-undefined
   CC = /opt/miyoo/usr/bin/arm-linux-gcc
   CXX = /opt/miyoo/usr/bin/arm-linux-g++
   AR = /opt/miyoo/usr/bin/arm-linux-ar
   PLATFORM_DEFINES += -D_GNU_SOURCE
   CFLAGS += -fomit-frame-pointer -ffast-math -march=armv5te -mtune=arm926ej-s
else
   CC ?= gcc
   CXX ?= g++
   TARGET := $(TARGET_NAME)_libretro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=link.T -Wl,--no-undefined
endif

LDFLAGS += $(LIBM)

ifeq ($(DEBUG), 1)
   CXXFLAGS += -O0 -g
else
   CXXFLAGS += -O3
endif

include Makefile.common

OBJECTS   := $(SOURCES_C:.c=.o) $(SOURCES_CXX:.cpp=.o)
CFLAGS    += -Wall -pedantic $(fpic)
CFLAGS    += $(INCFLAGS) $(INCFLAGS_PLATFORM)
CXXFLAGS  += -Wall -pedantic $(fpic)
CXXFLAGS  += $(INCFLAGS) $(INCFLAGS_PLATFORM)

ifneq (,$(findstring qnx,$(platform)))
CXXFLAGS += -Wc,-std=c++98
else
CXXFLAGS += -std=c++98
endif

OBJOUT   = -o 
LINKOUT  = -o 

ifneq (,$(findstring msvc,$(platform)))
	OBJOUT = -Fo
	LINKOUT = -out:
ifeq ($(STATIC_LINKING),1)
	LD ?= lib.exe
	STATIC_LINKING=0
else
	LD = link.exe
endif
else
	LD = $(CXX)
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(LD) $(fpic) $(SHARED) $(LINKOUT)$@ $(OBJECTS) $(LDFLAGS)
endif

# Track which headers each object was built from, so that editing one rebuilds
# everything that included it. Without this a change to a class definition
# leaves objects around that still allocate the old size of it, and the result
# is heap corruption at run time rather than an error at build time.
DEPS := $(OBJECTS:.o=.d)

%.o: %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $(fpic) -MMD -MP -c $(OBJOUT)$@ $<

%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) $(fpic) -MMD -MP -c $(OBJOUT)$@ $<

-include $(DEPS)

clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS)

.PHONY: clean


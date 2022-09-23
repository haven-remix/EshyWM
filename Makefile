CXXFLAGS ?= -Wall -g
CXXFLAGS += -std=c++20
CXXFLAGS += `pkg-config --cflags x11 libglog`
LDFLAGS += `pkg-config --libs x11 libglog`

#Constant variables
PROGRAM_NAME = eshywm
BUILD_DIR = ./build
BIN_DIR = ./bin

#Make all targets
all: $(PROGRAM_NAME)_compile $(PROGRAM_NAME)

#Source files
HEADERS = \
    util.h \
    config.h \
    window_data.h \
    window.h \
    window_manager.h \
    eshywm.h
SOURCES = \
    util.cpp \
    window_data.cpp \
    window.cpp \
    window_manager.cpp \
    eshywm.cpp \
    main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

#eshywm link target
$(PROGRAM_NAME): $(HEADERS) $(OBJECTS)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(PROGRAM_NAME) $(OBJECTS)
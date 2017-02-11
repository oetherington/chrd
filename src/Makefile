# Dependencies:
# - libharu (an up to date version, probably from source, to enable utf-8)
# - libz
# - libpng
# - gtk3
# - gtksourceview
# - evince

CPPSRC = $(wildcard *.cpp) $(wildcard csscolorparser/*.cpp)
CSRC = $(wildcard *.c) $(wildcard xxhash/*.c)
OBJ = $(CPPSRC:.cpp=.o) $(CSRC:.c=.o)
TARGET = chrd

FONTS = $(wildcard fonts/*.ttf)
FONTDATA = $(FONTS:.ttf=.inc)

ICON = icon/icon.png
ICONDATA = $(ICON:.png=.inc)

LANGS = $(wildcard syntax/*.lang)
RNGS = $(wildcard syntax/*.rng)
LANGDATA = $(LANGS:.lang=.inc) $(RNGS:.rng=.inc)

DUMMY = dummy/dummy.pdf
DUMMYDATA = $(DUMMY:.pdf=.inc)

LIBS = gtk+-3.0 evince-view-3.0 gtksourceview-3.0 gtk+-unix-print-3.0

COMMONFLAGS = -c -O3 -Wall -Wno-unused-parameter -D_GNU_SOURCE \
			  -D_FORTIFY_SOURCE=1 -I/usr/include/evince/3.0/ \
			  $(shell pkg-config --cflags $(LIBS))

CXX = clang++
CXXFLAGS = $(COMMONFLAGS) -std=c++14

CC = clang
CFLAGS = $(COMMONFLAGS) -std=c99 -Wno-unused-function

LD = clang++
LDFLAGS = -lm -lpthread /usr/local/lib/libhpdf.a -lz \
		  $(shell pkg-config --libs $(LIBS))

.PHONY: $(TARGET) clean run

all: debug

debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

release: CXXFLAGS += -DNDEBUG
release: LDFLAGS += -s
release: $(TARGET)

$(TARGET): $(FONTDATA) $(ICONDATA) $(LANGDATA) $(DUMMYDATA) $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

%.inc: %.ttf
	xxd -i $< > $@

%.inc: %.png
	xxd -i $< > $@

%.inc: %.lang
	xxd -i $< > $@

%.inc: %.rng
	xxd -i $< > $@

%.inc: %.pdf
	xxd -i $< > $@

clean:
	rm -rf $(OBJ) $(TARGET) $(FONTDATA) $(ICONDATA) $(LANGDATA) $(DUMMYDATA)

run: $(TARGET)
	./$(TARGET)

runv: $(TARGET)
	valgrind ./$(TARGET)

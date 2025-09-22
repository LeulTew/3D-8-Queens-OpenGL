CXX = g++
CXXFLAGS = -I/home/leul/include/stb -I/home/leul/include/tinyobjloader -I/usr/include/freetype2 -I/usr/include/AL
LDFLAGS = -lglut -lGLU -lGL -lfreetype -lopenal

TARGET = main
SOURCES = main.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
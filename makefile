
CXX = g++

CXXFLAGS = -std=c++17 -Iglad/include main.cpp glad/src/glad.c -lglfw -lGL -ldl -pthread

TARGET = a

SRCS = main.cpp glad/src/glad.c 

OBJS = 

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^


%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGET)
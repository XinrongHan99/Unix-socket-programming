CXX = g++

CFLAGS = -g -Wall
default = all
all: serverM serverC serverCS serverEE client

$(TARGET): $(TARGET).cpp
	$(CXX) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

serverM: serverM.cpp 
	$(CXX) $(CFLAGS) -o serverM serverM.cpp

serverC: serverC.cpp 
	$(CXX) $(CFLAGS) -o serverC serverC.cpp

serverCS: serverCS.cpp 
	$(CXX) $(CFLAGS) -o serverCS serverCS.cpp

serverEE: serverEE.cpp 
	$(CXX) $(CFLAGS) -o serverEE serverEE.cpp

client: client.cpp
	$(CXX) $(CFLAGS) -o client client.cpp
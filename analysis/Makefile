TARGET=bytecode_analysis
CXX=g++
CXXFLAGS=-O3 -g -m32 
BUILDS=../build
RUNTIME=../runtime
LAMA_ROOT=..
LAMAC=lamac
TEST_FILES=../performance
BYTERUN_FOLDER=../byterun
COMMON=common

all: $(TARGET)

.PHONY: all lama_runtime test mkbuild clean

lama_runtime: 
	$(MAKE) -C $(RUNTIME)

$(TARGET).o: $(TARGET).cpp mkbuild
	$(CXX) $(CXXFLAGS) -c $(TARGET).cpp -o $(BUILDS)/$(TARGET).o

$(COMMON).o: $(BYTERUN_FOLDER)/$(COMMON).c mkbuild
	$(CC) $(CXXFLAGS) -c $(BYTERUN_FOLDER)/$(COMMON).c -o $(BUILDS)/$(COMMON).o

$(TARGET): $(TARGET).o $(COMMON).o lama_runtime
	$(CXX) $(CXXFLAGS) $(BUILDS)/$(TARGET).o $(BUILDS)/$(COMMON).o -o $(BUILDS)/$(TARGET) 

test: $(TARGET)
	$(MAKE) -C $(TEST_FILES) bytecode_analysis

mkbuild: 
	mkdir -p $(BUILDS) 

clean:
	$(RM) -r build 
	$(MAKE) -C $(RUNTIME) clean 
 
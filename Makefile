export CXX=clang++
export LDFLAGS = -pthread -lm
export CFLAGS =  -std=c++11 -Wall -O2  -Wno-unknown-pragmas -funroll-loops\
	 -Iinclude -Idmlc-core/include -fPIC

# specify tensor path
.PHONY: clean all test doc

all: lib/libtvm.a lib/libtvm.so
SRC = $(wildcard src/*.cpp src/*/*.cpp)
ALL_OBJ = $(patsubst src/%.cpp, build/%.o, $(SRC))
ALL_DEP = $(ALL_OBJ)


test: $(TEST)

build/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) -MM -MT build/$*.o $< >build/$*.d
	$(CXX) -c $(CFLAGS) -c $< -o $@


lib/libtvm.a: $(ALL_DEP)
	@mkdir -p $(@D)
	ar crv $@ $(filter %.o, $?)

lib/libtvm.so: $(ALL_DEP)
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) -shared -o $@ $(filter %.o %.a, $^) $(LDFLAGS)


lint:
	python2 dmlc-core/scripts/lint.py tvm cpp include src

doc:
	doxygen docs/Doxyfile

clean:
	$(RM) -rf build lib bin *~ */*~ */*/*~ */*/*/*~ */*.o */*/*.o */*/*/*.o */*.d */*/*.d */*/*/*.d

-include build/*.d
-include build/*/*.d

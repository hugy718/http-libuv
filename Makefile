COMMON_CXX_FLAGS := -g -O0 -Wall -Wextra
# COMMON_CXX_FLAGS += -fsanitize=address

DEPS_INSTALL_DIR = deps/install
HTTP_PARSER_DIR = deps/http-parser
LIBUV_DIR = $(DEPS_INSTALL_DIR)/libuv

COMMON_CXX_FLAGS += -I$(HTTP_PARSER_DIR) -I$(LIBUV_DIR)/include

COMMON_LINK_FLAGS := $(COMMON_CXX_FLAGS)
COMMON_LINK_FLAGS += -L$(HTTP_PARSER_DIR) -l:libhttp_parser.a \
	-L$(LIBUV_DIR)/lib/ -l:libuv_a.a -lpthread -ldl

preparation:
	git submodule update --init --recursive

deps:
	mkdir deps/install
	cd deps/http-parser && make clean && make package && cd ../..
	cd deps/libuv && mkdir build && cd build && cmake .. -DCMAKE_INSTALL_PREFIX=../../install/libuv && make && make install && cd ../../..

.PHONY: preparation deps all clean mrproper

all: preparation deps sample-server

Sources := main.cc
Source_objects = $(Sources:.cc=.o)

%.o: %.cc
	$(CXX) $(COMMON_CXX_FLAGS) -c $< -o $@

sample-server: $(Source_objects)
	$(CXX) $^ -o $@ $(COMMON_LINK_FLAGS)

clean:
	rm -f *.o
	rm -f sample-server

mrproper: clean
	make -C deps/http-parser clean
	rm -rf deps/libuv/build
	rm -rf deps/install

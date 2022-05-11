# choose between llhttp and http_parser (default http_parser)
USE_LLHTTP ?= 0

COMMON_CXX_FLAGS := -g -O0 -Wall -Wextra
# COMMON_CXX_FLAGS += -fsanitize=address

DEPS_INSTALL_DIR = deps/install
HTTP_PARSER_DIR = deps/http-parser
LIBUV_DIR = $(DEPS_INSTALL_DIR)/libuv
LLHTTP_DIR = $(DEPS_INSTALL_DIR)/llhttp

COMMON_LINK_FLAGS := $(COMMON_CXX_FLAGS) \
	-L$(LIBUV_DIR)/lib -l:libuv_a.a -lpthread -ldl

COMMON_CXX_FLAGS += -I$(LIBUV_DIR)/include

ifneq ($(USE_LLHTTP), 0)
	COMMON_CXX_FLAGS += -DUSE_LLHTTP -I$(LLHTTP_DIR)/include
	COMMON_LINK_FLAGS += -L$(LLHTTP_DIR)/lib -l:libllhttp.a
else
	COMMON_CXX_FLAGS += -I$(HTTP_PARSER_DIR)
	COMMON_LINK_FLAGS += -L$(HTTP_PARSER_DIR) -l:libhttp_parser.a
endif

preparation:
	git submodule update --init --recursive

deps:
	mkdir $(DEPS_INSTALL_DIR)
	cd deps/http-parser && make clean && make package && cd ../..
	cd deps/libuv && mkdir build && cd build && cmake .. -DCMAKE_INSTALL_PREFIX=../../install/libuv && make && make install && cd ../../..
	mkdir -p $(DEPS_INSTALL_DIR)/llhttp/include && mkdir $(DEPS_INSTALL_DIR)/llhttp/lib 
	cd deps/llhttp && npm install && make && PREFIX=../install/llhttp make install && cd ../..

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
	make -C deps/llhttp clean
	rm -rf deps/libuv/build
	rm -rf deps/install

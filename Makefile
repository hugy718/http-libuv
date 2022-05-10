COMMON_CXX_FLAGS := -g -O0 -Wall -Wextra
# COMMON_CXX_FLAGS += -fsanitize=address

HTTP_PARSER_DIR = deps/http-parser

COMMON_CXX_FLAGS += -I$(HTTP_PARSER_DIR)

COMMON_LINK_FLAGS := $(COMMON_CXX_FLAGS)
COMMON_LINK_FLAGS += -luv -L$(HTTP_PARSER_DIR) -l:libhttp_parser.a

.PHONY: all clean

all: sample-server

Sources := main.cc
Source_objects = $(Sources:.cc=.o)

%.o: %.cc
	$(CXX) $(COMMON_CXX_FLAGS) -c $< -o $@

sample-server: $(Source_objects)
	$(CXX) $^ -o $@ $(COMMON_LINK_FLAGS)

clean:
	rm -f *.o
	rm -f sample-server

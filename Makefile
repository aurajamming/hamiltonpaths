GTEST_DIR = /usr/src/gtest

CPPFLAGS += -I$(GTEST_DIR)/include
CXXFLAGS +=  -std=c++11 -Wall -Wextra -pthread
#CXXFLAGS += -g -O0 -DDEBUG
CXXFLAGS += -g -O3 -DNDEBUG -g

TESTS = configuration_test

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = /usr/include/gtest/*.h \
                /usr/include/gtest/internal/*.h

COUNT_SOURCES = count_paths.cc grid.cc
COUNT_HEADERS = configuration.hh combinations.hh grid.hh range.hh vector_out.hh
count: $(COUNT_SOURCES) $(COUNT_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o count $(COUNT_SOURCES) -lgmp -lgmpxx

test : $(TESTS)
	echo $(TESTS)
	for t in $(TESTS); do ./$$t; done

clean :
	rm -f $(TESTS) gtest.a gtest_main.a *.o count




all.o: all.cc count_paths.cc grid.cc configuration.hh combinations.hh grid.hh range.hh vector_out.hh
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c all.cc


# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

# Builds a sample test.  A test should link with either gtest.a or
# gtest_main.a, depending on whether it defines its own main()
# function.


configuration_test.o : configuration_test.cc configuration.hh $(GTEST_HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c configuration_test.cc

configuration_test : configuration_test.o gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ -o $@

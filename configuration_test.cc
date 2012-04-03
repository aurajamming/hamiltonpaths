#include "configuration.hh"
#include "gtest/gtest.h"

#include <vector>
#include <utility>
#include <iostream>
#include "vector_out.hh"
using namespace std;

typedef pair<int, int> link_t;
typedef tuple<int, int, string> masked_link_t;


void test_op(Configuration &c, link_t op) {
  cerr << "  " << c.tostring() << " via " << make_pair(get<0>(op), get<1>(op)) << " to ";
  c.link(get<0>(op), get<1>(op));
  cerr << c.tostring() << endl;    
}

void test_op(Configuration &c, masked_link_t op) {
  cerr << "  " << c.tostring();
  cerr << " via " << make_pair(get<0>(op), get<1>(op));
  c.link(get<0>(op), get<1>(op));
  cerr << " (" << c.tostring() << " masked with " << get<2>(op) << " to ";

  vector<bool> mask_bool(c.size());
  transform(begin(get<2>(op)), end(get<2>(op)), begin(mask_bool),
	    [](char c) { return c!='0'; });
  c.mask(mask_bool);

  cerr << c.tostring() << endl;    
}


template<class T>
void test_ops(tuple<string, vector<T>, string> test) {
  string start, end;
  vector<T> operations;
  tie(start, operations, end) = test;

  cerr << "testing " << start << " via " << operations << " to " << end << endl;

  Configuration c(start);
  EXPECT_EQ(start, c.tostring());

  for(auto op : operations) {
    test_op(c, op);
  }

  EXPECT_EQ(end, c.tostring());
};

TEST(Configuration, link) {
  typedef tuple<string, vector<link_t>, string> test_t;
  vector<test_t> tests {
    test_t{"1221", {{2,3}}, "1100"},
    test_t{"120201", {{1,2},{3,5}}, "101000"},
    test_t{"1002332", {{0,2},{5,6}}, "0012200"},
    test_t{"12233", {{2,3}}, "12002"},
    test_t{"0000", {{1,2}}, "0110"},
    test_t{"0000", {{0,1},{2,3}}, "1122"},
    test_t{"1221", {{1,2}}, "1001"},
    test_t{"100", {{0,0}}, "100"},
    test_t{"000", {{0,0}}, "100"},
    test_t{"010", {{1,1}}, "010"},
    test_t{"000", {{1,1}}, "010"},
    test_t{"10220", {{0,1}, {3,4}}, "01202"},
    test_t{"1234432", {{2,3}, {5,6}}, "1200200"},
    test_t{"1202", {{0,1}}, "0001"},
  };

  for(auto t : tests) {
    test_ops(t);
  }
}

TEST(Configuration, mask) {
  typedef tuple<string, vector<masked_link_t>, string> test_t;
  vector<test_t> tests {
    test_t{"01202", { masked_link_t{0, 1, "10101"}, 
                      masked_link_t{2, 3, "10011"}}, 
	   "10022"},
  };

  for(auto t : tests) {
    test_ops(t);
  }
}

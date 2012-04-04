#ifndef __RANGE_HH__
#define __RANGE_HH__

#include <algorithm>

struct range {
  const int _start, _end;

  range(int start, int end) : _start(start), _end(end) {}
  range(int end) : range(0, end) {}

  struct range_iter : std::iterator<std::forward_iterator_tag, int, void> {
    int idx;
    range_iter(int i) : idx(i) {}
    void operator++() { ++idx; }
    bool operator!=(const range_iter &other) { return idx != other.idx; }
    int operator*() const { return idx; }
  };


  friend range_iter begin(range &r) {
    return range_iter(r._start);
  }
    
  friend range_iter end(range &r) {
    return range_iter(r._end);
  }
};





#endif

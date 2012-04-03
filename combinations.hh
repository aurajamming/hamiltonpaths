#ifndef __COMBINATIONS_HH__
#define __COMBINATIONS_HH__

// Modified from itertoolsmodule.c in the python 2.7 source
// TODO how to express license/copyright?


#include <vector>
#include <algorithm>
#include <cassert>
#include "range.hh"
#include "vector_out.hh"
using namespace std;


template<class T>
struct combinations {
  const vector<T> pool;
  const size_t r;

  combinations(const vector<T> elements, int r) :
    pool(elements),
    r(r)
  {
    assert(r>=0);
  }

  friend bool operator==(const combinations<T> &a, const combinations<T> &b) {
    return a.r == b.r and a.pool == b.pool;
  }
  friend bool operator!=(const combinations<T> &a, const combinations<T> &b) {
    return not a==b;
  }
};


template<class T>
struct combinations_iter : iterator<forward_iterator_tag, T, void> {
  bool stopped;
  const combinations<T> &parent;
  vector<size_t> indices;
  vector<T> result;

  combinations_iter(const combinations<T> &c) 
    : combinations_iter(c, c.r > c.pool.size())
  {}

  combinations_iter(const combinations<T> &c, bool stopped) :
    stopped(stopped),
    parent(c),
    indices(c.r),
    result(c.r)
  {
    iota(begin(indices), end(indices), 0);
    if (!stopped)
      initialize();
  }

  void initialize() {
    for(auto i : range(parent.r)) {
      T copy = parent.pool[indices[i]];      
      result[i] = copy; //parent.pool[indices[i]];
    }
  }

  void next() {
    if (stopped)
      return;

    int i, j;
    const size_t n = parent.pool.size();
    const size_t r = parent.r;

    for (i = r - 1 ; 
	 (i >= 0) && (indices[i] == (i + n - r)) ; 
	 i--)
      /* pass */ ;

    if (i >= 0) {
      indices[i] ++;
      for (j = i + 1 ; j < r ; j++)
	indices[j] = indices[j - 1] + 1;
    
      for ( ; i < r ; i++) {
	result[i] = parent.pool[indices[i]];
      }
    } else {
      stopped = true;
    }
  }

  friend bool operator==(const combinations_iter<T> &a, 
			 const combinations_iter<T> &b) {
    return (a.stopped == b.stopped and
	    a.parent == b.parent);
  }

  friend bool operator!=(const combinations_iter<T> &a, 
			 const combinations_iter<T> &b) {
    return not (a == b);
  }

  vector<T> operator*() const {
    return result;
  }

  void operator++() {
    next();
  }

  friend ostream &operator<<(ostream &os, const combinations_iter &it) {
    os << "<comb iter " << it.parent.pool << " r " << it.parent.r;
    os << " stopped: " << it.stopped;
    os << " indices: " << it.indices;
    os << " result: " << it.result;
    os << ">";
    return os;
  }

};


template<class T>
combinations_iter<T> begin(combinations<T> const &c) {
  return combinations_iter<T>(c);
}

template<class T>
combinations_iter<T> end(combinations<T> const &c) {
  return combinations_iter<T>(c, true);
}





#endif

#ifndef __VECTOR_OUT__
#define __VECTOR_OUT__

#include <vector>
#include <map>
#include <utility>
#include <iostream>
#include "range.hh"

template<class T>  
ostream &operator<<(ostream &os, const vector<T> &things) {
  bool first = true;
  os << "[";
  for(auto cur : things) {
    if (not first) {
      os << ", ";
    }
    first = false;
    os << cur;
  }
  os << "]";
  return os;
}

template<class T, size_t N>
ostream &operator<<(ostream &os, const array<T,N> &things) {
  bool first = true;
  os << "[";
  for(auto cur : things) {
    if (not first) {
      os << ", ";
    }
    first = false;
    os << cur;
  }
  os << "]";
  return os;
}

template<class T, class V>
ostream &operator<<(ostream &os, const pair<T,V> &things) {
  os << "(" << things.first << ", " << things.second << ")";
  return os;
}

template <size_t N, class T>
struct dump_tuple {
  static void dump(ostream &os, const T &t) {  
    dump_tuple<N-1, T>::dump(os, t);
    os << ", ";
    os << get<N-1>(t);
  }
};

template<class T>
struct dump_tuple<1, T> {
  static void dump(ostream &os, const T &t) {
    os << get<0>(t);
  }
};

template<class... Ts>
ostream &operator<<(ostream &os, const tuple<Ts...> &things) {
  os << "(";
  dump_tuple<sizeof...(Ts), const tuple<Ts...> >::dump(os, things);
  os << ")";
  return os;
}
  

template<class T1, class T2>
ostream &operator<<(ostream &os, const map<T1, T2> &things) {
  bool first = true;
  os << "{";
  for(auto cur : things) {
    if (not first) {
      os << ", ";      
    }
    first = false;
    os << cur.first << ": " << cur.second;
  }
  os << "}";

  return os;
}

#endif

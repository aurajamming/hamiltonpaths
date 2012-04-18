#ifndef __CONFIGURATION_HH_
#define __CONFIGURATION_HH_

#include <vector>
#include <array>
#include <unordered_set>
#include <utility>
#include <algorithm>
#include <limits>
#include <iostream>
#include <sstream>
#include <cassert>
#include <stdint.h>
#include "range.hh"

using namespace std;


struct no_size_t {
  inline operator size_t() const { 
    assert(false);
    return 0;
  } // never called

  inline no_size_t & operator=(const size_t &) { 
    assert(false);
    return *this; 
  } // never called
};


template<class T> struct container_details;

template<class T> struct container_details< vector<T> > { 
  static const size_t max_size = numeric_limits< size_t >::max();
  static const bool needs_member_size = false;

  template<class C>
  static void init(C &, vector<T> & container, size_t count, T const & value) {
    container = vector<T>(count, value);
  }
};

template<class T, size_t N> struct container_details< array<T,N> > {
  static const size_t max_size = N;
  static const bool needs_member_size = true;

  template<class C>
  static void init(C &config, array<T,N> & container, size_t count, T const & value) {
    assert(count < max_size);
    config._size = count;
    fill_n(begin(container), count, value);
  }
};

template<bool _>
class index_type {};
 

template<class container_type, 
	 class size_type=typename container_type::value_type, 
	 class col_type=typename container_type::value_type>
struct Configuration {

  // static members
  static const col_type no_partner = numeric_limits<col_type>::max();

  // instance members
  size_type _size;
  container_type config;

  
  // construction
  Configuration(const vector<int> &config_label) {
    col_type x = no_partner;
    container_details<container_type>::init(*this, config, config_label.size(), x);

    unordered_set<int> seen;
    for(auto col : range(config_label.size())) {
      auto label = config_label[col];
      if (label == 0) {
	config[col] = no_partner;
	continue;
      }
      
      auto other_it = find(begin(config_label) + col + 1, end(config_label), label);
      if (other_it != end(config_label)) {
	int other_col = distance(begin(config_label), other_it);
	config[col] = other_col;
	config[other_col] = col;
      } else if (seen.count(label) == 0) {
	config[col] = col;
      } else {
        assert(config[config[col]] == col);
      }

      assert(sanity_check());
      
      seen.insert(label);
    }

    assert(sanity_check());
  }

  Configuration(const string &config_label) {
    auto intify = [](const string &config_label) -> vector<int> {
      vector<int> out(config_label.size());
      transform(begin(config_label), end(config_label), begin(out),
		[](char c) { return c - '0'; });
      return out;
    };

    Configuration c(intify(config_label));
    swap(*this, c);
    assert(sanity_check());
  }

  Configuration(Configuration const &other) : _size(other._size), config(other.config) {}
  Configuration(Configuration const &&other) : _size(other._size), config(move(other.config)) {}


  // methods
  bool sanity_check() const {
    for(col_type i : range(size())) {
      assert((config[i] == no_partner) ||
	     (config[config[i]] == i));
    }

    return true;
  }

  string tostring() {
    ostringstream os;
    os << *this;
    return os.str();
  }

  void link(col_type col_a, col_type col_b);
  void mask(vector<bool> mask);

  inline bool link_would_close(col_type col_a, col_type col_b) const {
    assert(sanity_check());
assert(col_a < col_b);

return col_a == config[col_b];
  }

  inline bool col_advances(col_type col) const { return config[col] != no_partner; }

  inline size_t size() const {
    return size(index_type<container_details<container_type>::needs_member_size>());
  }
  inline size_t size(index_type<true>) const { return _size; }
  inline size_t size(index_type<false>) const { return config.size(); }

};


template <class container_type, class size_type, class col_type>
void Configuration<container_type, size_type, col_type>::link(col_type col_a, col_type col_b) {
  assert(sanity_check());
  assert(col_a <= col_b);

  col_type partner_a = config[col_a];
  col_type partner_b = config[col_b];

  const auto split = [&]() {
    config[col_a] = col_b;
    config[col_b] = col_a;
  };

  const auto close = [&]() {
    config[col_a] = config[col_b] = no_partner;
  };

  const auto adjust_path = [&](col_type partner, col_type col_from, col_type col_to) {
    config[col_from] = no_partner;
    if (partner == col_from) {
      config[col_to] = col_to;
    } else {
      config[partner] = col_to;
      config[col_to] = partner;
    }
  };

  const auto merge = [&]() {
    config[partner_a] = partner_b;
    config[partner_b] = partner_a;
    config[col_a] = config[col_b] = no_partner;
    if (partner_a == col_a) {
      config[partner_b] = partner_b;
    } else if (partner_b == col_b) {
      config[partner_a] = partner_a;
    }
  };

  if (partner_a == no_partner and partner_b == no_partner) {
    split();
  } else if (col_a == col_b) {
    // pass
  } else if (col_a == partner_b) {
    assert(col_b == partner_a);
    close();
  } else if (partner_a == no_partner) {
    adjust_path(partner_b, col_b, col_a);
  } else if (partner_b == no_partner) {
    adjust_path(partner_a, col_a, col_b);
  } else {
    merge();
  }

  assert(sanity_check());
}

template <class container_type, class size_type, class col_type>
void Configuration<container_type, size_type, col_type>::mask(vector<bool> vmask) 
{
  assert(sanity_check());
  assert(vmask.size() == size());

  for(col_type col : range(size())) {
    if (col == no_partner or vmask[col])
      continue;

    col_type partner = config[col];
    config[col] = no_partner;
    if (partner != no_partner and partner != col) {
      config[partner] = partner;
    }
  }

  assert(sanity_check());
}


template <class container_type, class size_type, class col_type>
inline bool operator==(const Configuration<container_type, size_type, col_type> &a, 
		       const Configuration<container_type, size_type, col_type> &b) {
  return a.config == b.config;
}

template <class container_type, class size_type, class col_type>
inline bool operator!=(const Configuration<container_type, size_type, col_type> &a, 
		       const Configuration<container_type, size_type, col_type> &b) {
  return not (a==b);
}

template <class container_type, class size_type, class col_type>
inline bool operator<(const Configuration<container_type, size_type, col_type> &a, 
		      const Configuration<container_type, size_type, col_type> &b) {
  return a.config < b.config;
}

template <class container_type, class size_type, class col_type>
ostream &operator<<(ostream &os, 
		    const Configuration<container_type, size_type, col_type> &config) 
{
  vector<int> path_ids(config.size(), 0);
  int next_label = 0;

  for(col_type col : range(config.size())) {
    auto partner = config.config[col];
    if (partner == Configuration<container_type, size_type, col_type>::no_partner)
      continue;
    if (partner < col) {
      path_ids[col] = path_ids[partner];
    } else {
      path_ids[col] = ++next_label;
    }
  }

  assert(next_label < 10);

  for(auto label : path_ids) {
    os << label;
  }

  return os;
}





namespace std {
  template <class container_type, class size_type, class col_type>
  struct hash<Configuration<container_type, size_type, col_type> > {
    inline size_t operator()(const Configuration<container_type, size_type, col_type> &config ) const {
      return accumulate(begin(config.config), end(config.config), 
			hash<size_t>()(config.config.size()),
			[&](size_t so_far, col_type partner) {
			  return so_far ^ partner;
			});		      
    };
  };
  template <class container_type, class size_type, class col_type>
  inline void swap(Configuration<container_type, size_type, col_type> &a, 
		   Configuration<container_type, size_type, col_type> &b) {
    swap(a.config, b.config);
    swap(a._size, b._size);
  };
}



#endif

#ifndef __CONFIGURATION_HH_
#define __CONFIGURATION_HH_

#include <vector>
#include <utility>
#include <algorithm>
#include <stdint.h>
using namespace std;


struct Configuration {
  typedef int col_t;
  static const col_t no_partner;

  vector<col_t> config;

  bool sanity_check() const;

  Configuration(const vector<int> &config_label);
  Configuration(const string &config_label);
  Configuration(Configuration const &other);
  Configuration(Configuration const &&other);
  string tostring();

  void link(int col_a, int col_b);
  void mask(vector<bool> mask);

  bool link_would_close(int col_a, int col_b) const;
  bool col_advances(int col) const;
  inline size_t size() const {
    return config.size();
  }
};


inline bool operator==(const Configuration &a, const Configuration &b) {
  return a.config == b.config;
}
inline bool operator!=(const Configuration &a, const Configuration &b) {
  return not (a==b);
}
inline bool operator<(const Configuration &a, const Configuration &b) {
  return a.config < b.config;
}

ostream &operator<<(ostream &os, const Configuration &config);

namespace std {
  template <>
  struct hash<Configuration> {
    inline size_t operator()(const Configuration &config ) const {
      return accumulate(begin(config.config), end(config.config), 
			hash<size_t>()(config.config.size()),
			[&](size_t so_far, Configuration::col_t partner) {
			  return so_far ^ partner;
			});		      
    };
  };
  template<>
  inline void swap(Configuration &a, Configuration &b) {
    swap(a.config, b.config);
  };
}

#endif

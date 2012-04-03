#include "configuration.hh"
#include "range.hh"
#include "vector_out.hh"

#include <unordered_set>
#include <map>
#include <iterator>
#include <algorithm>
#include <utility>
#include <cassert>
#include <iostream>
#include <sstream>

using namespace std;

const Configuration::col_t Configuration::no_partner = -1;

Configuration::Configuration(const vector<int> &config_label) :
  config(config_label.size(), no_partner)
{
  unordered_set<int> seen;

  for(auto col : range(config_label.size())) {
    auto label = config_label[col];
    if (label == 0)
      continue;
      
    auto other_it = find(begin(config_label) + col + 1, end(config_label), label);
    if (other_it != end(config_label)) {
      int other_col = distance(begin(config_label), other_it);
      config[col] = other_col;
      config[other_col] = col;
    } else if (seen.count(label) == 0) {
      config[col] = col;
    }

    assert(sanity_check());
      
    seen.insert(label);
  }

  assert(sanity_check());
}


vector<int> intify(const string &config_label) {
  vector<int> out(config_label.size());
  transform(begin(config_label), end(config_label), begin(out),
	    [](char c) { return c - '0'; });
  return out;
}

Configuration::Configuration(const string &config_label) : 
  config(0) 
{
  Configuration c(intify(config_label));
  swap(*this, c);
  assert(sanity_check());
}


Configuration::Configuration(Configuration const &other) :
  config(other.config)
{}

Configuration::Configuration(Configuration const &&other) :
  config(move(other.config))
{}

string Configuration::tostring() {
  ostringstream os;
  os << *this;
  return os.str();
}

ostream &operator<<(ostream &os, const Configuration &c) {  
  const auto &config = c.config;
  vector<int> path_ids(config.size(), 0);
  int next_label = 0;

  for(auto col : range(config.size())) {
    auto partner = config[col];
    if (partner == Configuration::no_partner)
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


void Configuration::link(int col_a, int col_b) {
  assert(sanity_check());
  assert(col_a <= col_b);

  col_t partner_a = config[col_a];
  col_t partner_b = config[col_b];

  const auto split = [&]() {
    config[col_a] = col_b;
    config[col_b] = col_a;
  };

  const auto close = [&]() {
    config[col_a] = config[col_b] = no_partner;
  };

  const auto adjust_path = [&](col_t partner, col_t col_from, col_t col_to) {
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



void Configuration::mask(vector<bool> vmask) {
  assert(sanity_check());
  assert(vmask.size() == config.size());

  for(auto col : range(config.size())) {
    if (col == no_partner or vmask[col])
      continue;

    col_t partner = config[col];
    config[col] = no_partner;
    if (partner != no_partner and partner != col) {
      config[partner] = partner;
    }
  }

  assert(sanity_check());
}


bool Configuration::link_would_close(int col_a, int col_b) const {
  assert(sanity_check());
  assert(col_a < col_b);
  
  col_t partner_b = config[col_b];
  return col_a == partner_b;
}

bool Configuration::col_advances(int col) const {
  return config[col] != no_partner;
}

bool Configuration::sanity_check(void) const {
  for(auto i : range(config.size())) {
    if(config[i] == no_partner)
      continue;
    assert(config[config[i]] == i);
  }

  return true;
}





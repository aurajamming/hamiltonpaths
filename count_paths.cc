#include <fstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <unordered_map>

#include <gmpxx.h>

#include "configuration.hh"
#include "grid.hh"
#include "combinations.hh"
#include "range.hh"
#include "vector_out.hh"

using namespace std;

typedef mpz_class CountT;
typedef Configuration<vector<unsigned short>, no_size_t> ResizableConfiguration;
typedef Configuration<array<unsigned short, 16>, unsigned short> FastFixedConfiguration;


void row_setup(Grid g, Grid::Node::ordinate_t row, 
	       vector<Grid::Node::degree_t> &target_degrees, 
	       vector< vector<Grid::Node> > &next_neighbors) 
{
  target_degrees.clear();
  next_neighbors.clear();

  for(Grid::Node::ordinate_t col : range(g.cols)) {
    target_degrees.emplace_back(g.target_degree(row, col));

      vector<Grid::Node> next_neighbors_col;
      for(Grid::Node neighbor : g.neighbors(row, col)) {
	if (neighbor.row > row or (neighbor.row == row and neighbor.col > col)) {
	  next_neighbors_col.push_back(neighbor);
	}
      }

      next_neighbors.push_back(next_neighbors_col);
  }
}


template<class ConfigurationT, bool skip_useless_combinations=true, bool early_rejection=true>
class for_each_next_config {
private:
  const Grid::Node::ordinate_t row, size;
  const ConfigurationT & last_config;
  const vector<vector<Grid::Node> > &next_neighbors;
  const function<void (const ConfigurationT&)> action;
  bool endpoint_row;

  vector<Grid::Node::degree_t> residual_degrees;
  vector<bool> vmask, hmask;


public:

  for_each_next_config(const int row, 
		       const ConfigurationT &last_config, 
		       const vector<Grid::Node::degree_t>& target_degrees, 
		       const vector<vector<Grid::Node> >& next_neighbors,
		       const function<void (const ConfigurationT&) > &action)
    : row(row), 
      size(last_config.size()), 
      last_config(last_config),
      next_neighbors(next_neighbors),
      action(action),
      residual_degrees(last_config.size()),
      vmask(last_config.size(), false),
      hmask(last_config.size(), false)
  {
    endpoint_row = false;
    for(auto col : range(size)) {
      residual_degrees[col] = target_degrees[col] - (last_config.col_advances(col) ? 1 : 0);
      endpoint_row = endpoint_row or (residual_degrees[col] == 1);
    }

    if (early_rejection) {
      ConfigurationT start(last_config);
      enumerate_options_early_reject(0, 0, start);
    } else {
      enumerate_options_late_reject(0);
    }
    
  }

  void enumerate_options_late_reject(Grid::Node::ordinate_t col) {
    assert(col < size);

    const auto r = residual_degrees[col];
    if (skip_useless_combinations and r <= 0) {
      hmask[col] = vmask[col] = false;
      if (col == size - 1) {
	yield_configuration();
      } else {
	enumerate_options_late_reject(col + 1);
      }      
    }

    for(auto neighbor_comb : combinations<Grid::Node>(next_neighbors[col], (r<0)?0:r)) {
      hmask[col] = vmask[col] = false;
			   
      for(Grid::Node neighbor : neighbor_comb) {
	--residual_degrees[col];
	if (neighbor.row == row) {
	  --residual_degrees[col + 1];
	  hmask[col] = true;
	} else {
	  vmask[col] = true;
	}
      }


      if (col == size - 1) {
	yield_configuration();
      } else {
	enumerate_options_late_reject(col + 1);
      }

      for(Grid::Node neighbor : neighbor_comb) {
	++residual_degrees[col];
	if (neighbor.row == row) {
	  ++residual_degrees[col + 1];
	}
      }
    }
  }

  void enumerate_options_early_reject(Grid::Node::ordinate_t col, Grid::Node::ordinate_t start, ConfigurationT &config) {
    assert(col < size);

    bool skip, link;
    Grid::Node::ordinate_t a,b;

    const auto r = residual_degrees[col];
    if (skip_useless_combinations and early_rejection and r <= 0) {
      hmask[col] = vmask[col] = false;
      skip = determine_link_behavior(col, config, start, link, a, b);
      if (not skip) {
	if (link) {
	  ConfigurationT new_config(config);
	  new_config.link(a, b);
	  if (col == size - 1) {
	    yield_configuration(new_config);
	  } else {
	    enumerate_options_early_reject(col + 1, start, new_config);
	  }
	} else {
	  if (col == size - 1) {
	    yield_configuration(config);
	  } else {
	    enumerate_options_early_reject(col + 1, start, config);
	  }
	}
      }
      return;
    }

    for(auto neighbor_comb : combinations<Grid::Node>(next_neighbors[col], (r<0)?0:r)) {
      hmask[col] = vmask[col] = false;
			   
      for(Grid::Node neighbor : neighbor_comb) {
	--residual_degrees[col];
	if (neighbor.row == row) {
	  --residual_degrees[col + 1];
	  hmask[col] = true;
	} else {
	  vmask[col] = true;
	}
      }


      skip = determine_link_behavior(col, config, start, link, a, b);
      if (not skip) {
	if (link) {
	  ConfigurationT new_config(config);
	  new_config.link(a, b);

	  if (col == size - 1) {
	    yield_configuration(new_config);
	  } else {
	    enumerate_options_early_reject(col + 1, start, new_config);
	  }	  
	} else {
	  if (col == size - 1) {
	    yield_configuration(config);
	  } else {
	    enumerate_options_early_reject(col + 1, start, config);
	  }
	}
      }

      for(Grid::Node neighbor : neighbor_comb) {
	++residual_degrees[col];
	if (neighbor.row == row) {
	  ++residual_degrees[col + 1];
	}
      }
    }
  }

  inline bool determine_link_behavior(Grid::Node::ordinate_t col, const ConfigurationT & config, 
				      Grid::Node::ordinate_t & start,
				      bool & link, 
				      Grid::Node::ordinate_t & a, Grid::Node::ordinate_t & b) const {
    bool skip = false;
    link = false;
    if (hmask[col] and (col == 0 or not hmask[col-1])) {
      start = col;
    } else if (not hmask[col] and col > 0 and hmask[col-1]) {
      if (config.link_would_close(start, col)) {
	skip=true; // reject this configuration
      } else {	
	link = true;
	a = start;
	b = col;
      }
    } else if (vmask[col]) {
      link = true;
      a = col;
      b = col;
    }
    
    return skip;
  }

  inline void yield_configuration() const {
    // late rejection, do linkage here
    ConfigurationT config(last_config);
    for (Grid::Node::ordinate_t col : range(size)) {
      bool link;
      Grid::Node::ordinate_t start, a, b;
      bool skip = determine_link_behavior(col, config, start, link, a, b);
      if (skip)
	return;
      if (link) 
	config.link(a,b);
    }

    yield_configuration(config);
  }


  inline void yield_configuration(ConfigurationT & config) const {
    // early rejection, linkage done
    if (endpoint_row)
      config.mask(vmask);
      
    action(config);
    
  }
};


template <typename CountT>
struct configstate {
  array<CountT, 2> count;
  array<unsigned int, 2> tag;
};

template<typename CountT>
ostream &operator<<(ostream &os, const configstate<CountT> &cs) {
  os << "<count: " << cs.count << ", ";
  os << "tag: " << cs.tag << ">";
  return os;
}

			  
template<class ConfigurationT, class CountT>
CountT count_paths(Grid g) {
  typedef map<ConfigurationT, configstate<CountT> > config_set;

  config_set cur_configs, next_configs;
  vector<Grid::Node::degree_t> target_degrees(g.cols, -1);
  vector<vector<Grid::Node> > next_neighbors(g.cols);
  int sel = 1;

  const ConfigurationT empty_config(vector<int>(g.cols, 0));
  configstate<CountT> initial_state;
  initial_state.count[sel] = 1;
  initial_state.tag[sel] = 0;
  cur_configs.insert(make_pair(empty_config, initial_state));



  // cerr << "Initial config:" << endl;
  // cerr << cur_configs << endl;

  for(unsigned int row : range(g.rows)) {
    row_setup(g, row, target_degrees, next_neighbors);
    
    for(auto cur_config_count : cur_configs) {
      const auto & cur_config = cur_config_count.first;
      const auto & cur_count = cur_config_count.second.count[sel];

      if (cur_config_count.second.tag[sel] != row)
	continue;

      for_each_next_config<ConfigurationT>(row, cur_config, target_degrees, next_neighbors,
        [&](const ConfigurationT &next_config) {
          auto & count_state = cur_configs[next_config];

	  if (count_state.tag[1-sel] != row + 1) {
	    count_state.tag[1-sel] = row + 1;
	    count_state.count[1-sel] = 0;
	  }
	  count_state.count[1-sel] += cur_count;      

	  //next_configs[next_config] += cur_count;
        });
    }

    // swap(cur_configs, next_configs);
    // next_configs.clear();
    sel = 1 - sel;

    // cerr << "Configs after row " << row << ": " << endl;
    // cerr << cur_configs << endl;
  }

  const auto & closed_off = cur_configs[empty_config];
  if (closed_off.tag[sel] != g.rows) {
    // Failed to close off configuration sequence - no paths found.
    return 0;
  }

  return closed_off.count[sel];
}



template<typename F>
void repeat(size_t times, F action) {
  for(size_t i = times; i != 0; --i) 
    action();
}





int main(int argc, char *argv[]) {
  const bool use_file = argc > 1;
  ifstream file;  



  if (use_file) {
    file.open(argv[1]);
    if(not file.is_open()) {
      cerr << "Couldn't open '" << argv[1] << "'" << endl;
      return 1;
    }
  }

  int count = 1;
  if (argc > 2) {
    count = atoi(argv[2]);
  }

  Grid g(use_file ? file.seekg(0) : cin);

  CountT total = 0;

  
  if (g.cols <= 8) {
    repeat(count, [&]{total = count_paths<FastFixedConfiguration, CountT>(g);});
  } else {
    repeat(count, [&]{total = count_paths<ResizableConfiguration, CountT>(g);});
  }

  cout << total << endl;

  return 0;
}

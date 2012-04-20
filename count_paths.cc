#include <fstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <unordered_map>

#include "configuration.hh"
#include "grid.hh"
#include "combinations.hh"
#include "range.hh"
#include "vector_out.hh"

using namespace std;

typedef Configuration<vector<unsigned short>, no_size_t> ResizableConfiguration;
typedef Configuration<array<unsigned short, 8>, unsigned short> Max8Configuration;


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


template<class ConfigurationT>
class for_each_next_config {
private:
  const Grid::Node::ordinate_t row, size;
  const ConfigurationT &last_config;
  const vector<vector<Grid::Node> > &next_neighbors;
  const function<void (const ConfigurationT&)> action;

  vector<Grid::Node::degree_t> residual_degrees;
  vector<bool> vmask, hmask;

public:

  for_each_next_config(const int row_, 
		       const ConfigurationT &last_config_, 
		       const vector<Grid::Node::degree_t>& target_degrees_, 
		       const vector<vector<Grid::Node> >& next_neighbors_,
		       const function<void (const ConfigurationT&) > &action_)
    : row(row_), 
      size(last_config_.size()), 
      last_config(last_config_), 
      next_neighbors(next_neighbors_), 
      action(action_),
      residual_degrees(last_config_.size()),
      vmask(last_config_.size(), false),
      hmask(last_config_.size(), false)
  {
    for(auto col : range(size)) {
      residual_degrees[col] = target_degrees_[col] - (last_config.col_advances(col) ? 1 : 0);
    }

    enumerate_options(0);
  }

  void enumerate_options(Grid::Node::ordinate_t col) {
    assert(col < size);

    const auto r = residual_degrees[col];
    if (r <= 0) {
      if (col == size - 1) {
    	yield_configuration();
      } else {
	hmask[col] = vmask[col] = false;
    	enumerate_options(col + 1);
      }
      return;
    }
      

    for(auto neighbor_comb : combinations<Grid::Node>(next_neighbors[col], r)) {
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
	enumerate_options(col + 1);
      }

      for(Grid::Node neighbor : neighbor_comb) {
	++residual_degrees[col];
	if (neighbor.row == row) {
	  ++residual_degrees[col + 1];
	}
      }
    }
  }

  inline void yield_configuration() const {
    ConfigurationT config(last_config);
    int start = -1;

    for(auto col : range(size)) {
      if (hmask[col] and (col == 0 or not hmask[col-1])) {
	start = col;
      } else if (hmask[col] == 0 and col > 0 and hmask[col-1]) {
	if (config.link_would_close(start, col)) {
	  return; // reject this configuration
	}
	config.link(start, col);
      } else if (vmask[col]) {
	config.link(col, col);
      }
    }

    config.mask(vmask);

    action(config);
  }
};



struct configstate {
  array<unsigned int, 2> count;
  array<unsigned int, 2> tag;
};

ostream &operator<<(ostream &os, const configstate &cs) {
  os << "<count: " << cs.count << ", ";
  os << "tag: " << cs.tag << ">";
  return os;
}

			  
template<class ConfigurationT>
unsigned int count_paths(Grid g) {
  typedef map<ConfigurationT, configstate > config_set_t;
  typedef typename config_set_t::value_type config_count_t;

  config_set_t cur_configs, next_configs;
  vector<Grid::Node::degree_t> target_degrees(g.cols, -1);
  vector<vector<Grid::Node> > next_neighbors(g.cols);
  int sel = 1;

  ConfigurationT initial_config(vector<int>(g.cols, 0));
  configstate initial_state;
  initial_state.count[sel] = 1;
  initial_state.tag[sel] = 0;
  cur_configs.insert(make_pair(initial_config, initial_state));



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
          configstate & count_state = cur_configs[next_config];

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

  return accumulate(begin(cur_configs), end(cur_configs), 0, 
		    [&](unsigned int sum, config_count_t tagged_count) { 
		      if (tagged_count.second.tag[sel] != g.rows)
			return sum;
		      return sum + tagged_count.second.count[sel];
		    });
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

  unsigned int total = 0;

  
  if (g.cols <= 8) {
    repeat(count, [&]{total = count_paths<Max8Configuration>(g);});
  } else {
    repeat(count, [&]{total = count_paths<ResizableConfiguration>(g);});
  }

  cout << total << endl;

  return 0;
}

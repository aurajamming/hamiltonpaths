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


typedef unordered_map<Configuration, uint32_t> config_set_t;
typedef config_set_t::value_type config_count_t;


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



class for_each_next_config {
private:
  const Grid::Node::ordinate_t row, size;
  const Configuration last_config;
  const vector<vector<Grid::Node> > next_neighbors;
  const function<void (Configuration)> action;

  vector<Grid::Node::degree_t> residual_degrees;
  vector<bool> vmask, hmask;

public:

  for_each_next_config(const int row_, 
		       const Configuration last_config_, 
		       const vector<Grid::Node::degree_t>& target_degrees_, 
		       const vector<vector<Grid::Node> > next_neighbors_,
		       const function<void (Configuration) > &action_)
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
    if (col == size) {
      yield_configuration();
      return;
    }

    for(auto neighbor_comb : combinations<Grid::Node>(next_neighbors[col], 
						      max(Grid::Node::degree_t(0), residual_degrees[col]))) 
    {
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

      enumerate_options(col + 1);

      for(Grid::Node neighbor : neighbor_comb) {
	++residual_degrees[col];
	if (neighbor.row == row) {
	  ++residual_degrees[col + 1];
	}
      }
    }
  }

  void yield_configuration() {
    Configuration config(last_config);
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



			  

int count_paths(Grid g) {
  config_set_t cur_configs, next_configs;
  vector<Grid::Node::degree_t> target_degrees(g.cols, -1);
  vector<vector<Grid::Node> > next_neighbors(g.cols);

  Configuration initial_config(vector<int>(g.cols, 0));
  cur_configs.insert(make_pair(initial_config, 1));

  for(auto row : range(g.rows)) {
    row_setup(g, row, target_degrees, next_neighbors);
    
    for(auto cur_config_count : cur_configs) {
      const Configuration &cur_config = cur_config_count.first;
      const int cur_count = cur_config_count.second;

      for_each_next_config(row, cur_config, target_degrees, next_neighbors,
			   [&](const Configuration &next_config) {
			     next_configs[next_config] += cur_count;
			   });
    }

    swap(cur_configs, next_configs);
    next_configs.clear();
  }

  return accumulate(begin(cur_configs), end(cur_configs), 0, 
		    [](int sum, config_count_t config_count_t) { 
		      return sum + config_count_t.second; 
		    });
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

  int total = 0;
  for(auto i : range(count)) {
    total = count_paths(g);
  }

  cout << total << endl;

  return 0;
}

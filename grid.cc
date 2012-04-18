#include "grid.hh"

#include <algorithm>
#include <iterator>
#include <cassert>
#include "range.hh"

namespace std {
  template<>
  void swap(Grid &a, Grid &b) {
    swap(a.rows, b.rows);
    swap(a.cols, b.cols);
    swap(a.start_idx, b.start_idx);
    swap(a.end_idx, b.end_idx);
    swap(a.have_start_and_end, b.have_start_and_end);
    swap(a.nodes, b.nodes);
    swap(a.adjacency, b.adjacency);
  }
}

Grid::Grid(size_t rows, size_t cols) :
  rows(rows),
  cols(cols),
  have_start_and_end(false)
{
  nodes.reserve(rows * cols);
  for(Node::ordinate_t row : range(rows)) {
    for(Node::ordinate_t col : range(cols)) {
      assert(nodes.size() == index(row, col));
      assert(coordinates(nodes.size()) == Node::coordinate_t({row, col}));
#ifdef DEBUG
#define DELETED_INITIALIZER false
#else
#define DELETED_INITIALIZER
#endif
      nodes.emplace_back(Node({row, col, 2, DELETED_INITIALIZER}));
#undef DELETED_INITIALIZER
    }

  }
  
  adjacency.resize(nodes.size());

  for(Node::ordinate_t row : range(rows)) {
    for(Node::ordinate_t col : range(cols)) {      
      Node::coordinate_t pos(row, col);
      const Node::index_t idx = index(row, col);      
      if (row + 1 < rows) {
	const Node::index_t other_idx = index(row+1, col);
	adjacency[idx].push_back(other_idx);
	adjacency[other_idx].push_back(idx);
      }
      
      if (col + 1 < cols) {
	const Node::index_t other_idx = index(row, col+1);
	adjacency[idx].push_back(other_idx);
	adjacency[other_idx].push_back(idx);
      }
    }
  }
}


Grid::Grid(istream &is) {
  size_t rows, cols;
  unsigned int code;
  bool have_start(false), have_end(false);

  is >> cols;
  is >> rows;
  Grid g(rows, cols);

  for (auto row : range(rows)) {
    for (auto col: range(cols)) {
      is >> code;
      switch(code) {
      case 0:
	assert(g.target_degree(row, col) == 2);
	break;
      case 1:
	g.target_degree(row, col) = 0;
	g.delete_node(g.index(row, col));
	break;
      case 2:
	have_start = true;
	g.target_degree(row, col) = 1;
	g.start_idx = g.index(row, col);
	break;
      case 3:
	have_end = true;
	g.target_degree(row, col) = 1;
	g.end_idx = g.index(row, col);
	break;
      default:
	assert(false);
      }
    }
  }

  assert(have_start == have_end);
  g.have_start_and_end = have_start;

  swap(g, *this);
}


void Grid::print() const {
  auto sym = [&](Node::coordinate_t pos) { 
    auto idx = index(pos);
    if (idx == start_idx)
      return 'A';
    else if (idx == end_idx)
      return 'B';
#ifdef DEBUG
    else if (nodes[idx].deleted)
      return ' ';
#endif
    else
      return '+';
  };

  auto fwd = [&](Node::coordinate_t pos) {
    Node::coordinate_t fwd_pos(pos);
    fwd_pos.second++;
    if (connected(pos, fwd_pos))
      return '-';
    else
      return ' ';
  };

  auto down = [&](Node::coordinate_t pos) {
    Node::coordinate_t down_pos(pos);
    down_pos.first++;
    if (connected(pos, down_pos))
      return '|';
    else
      return ' ';
  };

  auto degr = [&](Node::coordinate_t pos) {
    auto foo(*this); // make a copy to avoid needing non-const this
    return (char)('0' + foo.target_degree(pos));
  };


  for(Node::ordinate_t row : range(rows)) {
    for(Node::ordinate_t col : range(cols)) {
      Node::coordinate_t pos(row, col);
      cout << sym(pos) << degr(pos) << fwd(pos);
    }
    cout << endl;
    for(Node::ordinate_t col : range(cols)) {
      Node::coordinate_t pos(row, col);
      cout << down(pos) << "  ";
    }
    cout << endl;
  }
      
  
}

void Grid::delete_node(Node::index_t idx) {
  for (auto other_idx : adjacency[idx]) {
    auto &other_children = adjacency[other_idx];
    other_children.erase(remove(begin(other_children), 
				end(other_children), 
				idx),
			 end(other_children));
  }
  adjacency[idx].clear();
#ifdef DEBUG
  nodes[idx].deleted = true;
#endif
}


Grid::Node::degree_t &Grid::target_degree(Grid::Node::coordinate_t pos) {
  return target_degree(pos.first, pos.second);
}

Grid::Node::degree_t &Grid::target_degree(Grid::Node::ordinate_t row, 
					  Grid::Node::ordinate_t col) {
  return nodes[index(row, col)].target_degree;
}

const vector<Grid::Node> Grid::neighbors(Grid::Node::coordinate_t pos) {
  return neighbors(pos.first, pos.second);
}

const vector<Grid::Node> Grid::neighbors(Grid::Node::ordinate_t row, 
					  Grid::Node::ordinate_t col) {
  assert(index(row, col) < adjacency.size());

  vector<Grid::Node> out;
  for(auto idx : adjacency[index(row, col)]) {
    out.push_back(nodes[idx]);
  }

  return out;
}


bool Grid::connected(Grid::Node::coordinate_t a, Grid::Node::coordinate_t b) const {
  if (not valid_coord(a) or not valid_coord(b))
    return false;

  const vector<Node::index_t> &neighbors = adjacency[index(a)];
  auto it = find(begin(neighbors), end(neighbors), index(b));
  return it != end(neighbors);
}

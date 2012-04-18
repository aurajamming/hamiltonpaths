#ifndef __GRID_HH__
#define __GRID_HH__

#include <iostream>
#include <memory>
#include <vector>
#include <utility>
#include <stdint.h>
using namespace std;

struct Grid {
  struct Node {
    typedef uint8_t ordinate_t;
    typedef uint8_t index_t;
    typedef int8_t degree_t; // may become negative
    typedef pair<ordinate_t, ordinate_t> coordinate_t;

    ordinate_t row, col;
    degree_t target_degree;
#ifdef DEBUG
    bool deleted;
#endif
    
    friend bool operator==(const Grid::Node &a, const Grid::Node &b) {
      return a.row == b.row && a.col == b.col;
    }

    friend ostream &operator<<(ostream &os, const Node &n) {
      os << "<Node (" << n.row << ", " << n.col << ") @ " 
	 << " degree " << n.target_degree;
#ifdef DEBUG
      if (n.deleted)
	os << " deleted";
#endif
      os << ">";
      return os;
    }
  };

  size_t rows, cols;
  Node::index_t start_idx, end_idx;
  bool have_start_and_end;

  vector<Node> nodes;
  vector< vector<Node::index_t> > adjacency;


  Grid(size_t rows, size_t cols);
  Grid(istream &is);

  void delete_node(Node::index_t idx);

  Node::degree_t &target_degree(Node::coordinate_t pos);
  Node::degree_t &target_degree(Node::ordinate_t row, Node::ordinate_t col);

  const vector<Node> neighbors(Node::coordinate_t pos);
  const vector<Node> neighbors(Node::ordinate_t row, Node::ordinate_t col);  

  bool connected(Node::coordinate_t a, Node::coordinate_t b) const;

  void print() const;


  inline bool valid_coord(const Node::coordinate_t &pos) const {
    return (pos.first >= 0 and rows > pos.first and
            pos.second >= 0 and cols > pos.second);
  }

  inline bool valid_coord(const Node::ordinate_t row, const Node::ordinate_t col) const {
    return valid_coord(Node::coordinate_t(row, col));
  }

  inline Node::index_t index(Node::ordinate_t row, Node::ordinate_t col) const {
    return (row * cols) + col;
  }

  inline Node::index_t index(Node::coordinate_t coord) const {
    return index(coord.first, coord.second);
  }

  inline Node::coordinate_t coordinates(Node::index_t idx) const {
    return make_pair(idx / cols, idx % cols);
  }




};



#endif

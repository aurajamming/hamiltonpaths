from collections import defaultdict
from itertools import combinations
import networkx as nx
import timeit
import sys

from read_graph import read_graph

#from endmapconfig import EndMapConfig as Config
from partnerconfig import PartnerConfig as Config

def next_configs(row, last_config, target_degrees, next_neighbors):
    residual_degrees = [target_degrees[col] - (0 if lineid == 0 else 1) 
                        for (col, lineid) in enumerate(last_config)]

    last_config = Config(last_config)
    cols = len(last_config)

    # the vmask and hmask arrays are declared here as mutable arrays
    # to avoid them being passed as arguments to the options
    # function. Ordinarily this would be stupid, but options is on the
    # hot path and avoiding passing them around and allocating new
    # arrays at each call makes a noticeable difference in speed
    # without the code getting too obtuse.
    vmask = [0] * cols
    hmask = [0] * cols
    
    def options(col=0):
        # Recursively enumerate every possible next configuration from
        # this configuration and row.

        if col == cols:
            # base case: a decision has been made for every forward-
            # and down-edge in this row. Use this information to
            # extract horizontal links and create a configuration for
            # the resulting next row.

            config = last_config.copy()

            for idx in range(cols):
                if hmask[idx] == 1 and (idx == 0 or hmask[idx-1] == 0):
                    # start of horizontal link
                    start = idx 
                elif hmask[idx] == 0 and idx > 0 and hmask[idx-1] == 1:
                    # end of horizontal link                    
                    if config.would_close(start, idx):
                        # if doing this link would form a cycle,
                        # reject this configuration by returning
                        # before yielding any configuration
                        return
                    # otherwise, record the link
                    config.link(start, idx)
                elif vmask[idx] == 1:
                    # straight link from previous to next row
                    config.link(idx, idx)

            config.mask(vmask)

            yield config
            return

        # otherwise, we have to enumerate each possible set of
        # forward- and down-edges from column number col
        for neighbor_comb in combinations(next_neighbors[col], max(0, residual_degrees[col])):

            # maintain shared state 
            hmask[col] = vmask[col] = 0
            for neighbor in neighbor_comb:
                residual_degrees[col] -= 1
                if neighbor[0] == row:
                    assert neighbor[1] == col + 1
                    residual_degrees[col + 1] -= 1
                    hmask[col] = 1
                else:
                    vmask[col] = 1

            # recurse to get all the completed configurations from this point.
            for completed in options(col + 1):
                yield completed

            # undo changes to shared state
            for neighbor in neighbor_comb:
                residual_degrees[col] += 1
                if neighbor[0] == row:
                    residual_degrees[col + 1] += 1


    # start the recursion and yield each configuration as a tuple
    for option_config in options():
        yield tuple(option_config.path_ids())


def row_setup(g, row):
    """ The next_configs function is guided by trying to satisfy the
        target degree of each node (1 for start and end nodes, 2 all
        others) using the available forward- and down-edges from that
        node.

        This is called once per row; it is not performance-sensitive.
    """

    cols = g.graph['cols']

    target_degrees = [(g.node[(row, col)]['target_degree']
                       if (row, col) in g
                       else 0)  
                      for col in range(cols)]

    # read row_, col_ as "row prime, col prime"
    next_neighbors = [ [(row_, col_)
                        for (row_, col_) in (g.neighbors_iter( (row, col) )
                                             if (row, col) in g
                                             else [])
                        if (row_ > row
                            or (row_ == row and col_ > col))]
                       for col in range(cols)]

    return target_degrees, next_neighbors


def mk_option_graph(g, configs, last_row=None):
    """ Given: g - the original grid graph returned by read_graph
               configs - a sequence of configurations in tuple format
               last_row - the row number that configs[-1] corresponds
                          to, or None if configs represents every row.

        Return a graph of the paths implied by the given
        configurations over the associated rows.

        This is used for show_paths and debug visualization.
    """

    rows = g.graph['rows']
    cols = g.graph['cols']
    g2 = nx.create_empty_copy(g)
    num_configs = len(configs)

    if last_row is None:
        last_row = rows - 1
    
    for row, config in zip(range(last_row - num_configs + 1, last_row + 1), 
                           configs):
        target_degrees, next_neighbors = row_setup(g, row)

        # vertical edges are easy to infer from a configuration
        for col, line_id in enumerate(config):
            if line_id != 0:
                g2.add_edge( (row, col), (row+1, col) )

        # horizontal edges can be recovered because nodes are
        # constrained to their target_degrees:
        for col, line_id in enumerate(config):
            if g2.has_node( (row, col) ):
                if len(g2.neighbors((row, col))) < target_degrees[col]:
                    g2.add_edge( (row, col), (row, col+1) )

    return g2


def print_configs(g, configs, last_row=None):
    """ Given a list of configs (corresponding to sequential rows
        ending at last_row or the last row of the graph, display a
        text representation of the path they collectively represent.

        Useful for visualization and debugging.
    """


    rows = g.graph['rows']
    cols = g.graph['cols']
    g2 = mk_option_graph(g, configs, last_row)

    def node_sym(row, col):
        """ What ASCII symbol to use for this node """
        if (row, col) not in g:
            return " "
        if (row, col) == g.graph['start_node']:
            return "A"
        if (row, col) == g.graph['end_node']:
            return "B"
        return "+"

    # This might be a bit condensed, but basically we just draw the
    # node, along with "-" beside it if it has a forward-edge and/or
    # "|" if it has a down-edge.
    for row in range(rows):
        print "".join("%s%s" % (node_sym(row, col), ("-" if g2.has_edge( (row, col), (row, col+1)) else " "))
                      for col in range(cols))
        print "".join("%s " % ("|" if g2.has_edge( (row, col), (row+1, col)) else " ")
                      for col in range(cols))
    print


def print_validation(g, configs):
    """ Print out warning messages if the given list of configurations
        does not represent a valid hamiltonian path from start to end
        nodes.
        
        Returns True if the path is valid, False otherwise.
        
        This is a debugging/verification function, not part of the
        main algorithm.
    """

    g2 = mk_option_graph(g, configs)
    start = g.graph['start_node']
    end = g.graph['end_node']

    errors = []

    for node in g2.nodes_iter():
        degree = g2.degree(node)
        target = g.node[node]['target_degree']
        if degree != target:
            errors.append("Node %s has degree %d, should be %d" % (node, degree, target))
    
    try:
        path = nx.shortest_path(g2, source=start, target=end)
    except nx.NetworkNoPath:
        path = None
        errors.append("No path between start and end nodes")

    if path:
        if len(path) != g.number_of_nodes():
            errors.append("Path does not include all nodes")

    if errors:
        print "FAILED PATH"
        for c in configs:
            print "  %s" % str(c)
        for e in errors:
            print "  %s" % e

    return not bool(errors)
    


def show_paths(g, bad_only = False):
    """ A variant of count_paths that displays a text representation
        of each path as it is discovered. This will take much longer
        to run, as it cannot condense equivalent configurations as
        count_paths does; but it's memory usage is kept reasonable by
        using generators to only store the relevant configurations
        during the search.

        As a debugging for large graphs, passing in bad_only=True will
        cause this to only output paths which fail validation.
    """


    cols = g.graph['cols']
    rows = g.graph['rows']
    start_config = (0,) * cols

    def all_paths_from(row, config):
        if row == rows:
            yield []
            return

        target_degrees, next_neighbors = row_setup(g, row)
        for next_config in next_configs(row, config, target_degrees, next_neighbors):
            for rest_configs in all_paths_from(row+1, next_config):
                yield [next_config] + rest_configs

    for count, configs in enumerate(all_paths_from(0, start_config)):
        ok = print_validation(g, configs)
        if not bad_only or not ok:
            print "Path %d:" % (count + 1)
            print_configs(g, configs)

def count_paths(g):
    """ Given a grid graph g in the format returned by read_graph,
        return the number of hamiltonian paths from it's marked start
        node to it's end node.

        This uses a mapping of configurations to counts. Since we
        don't care HOW a given configuration is reached, but only it's
        connectivity (which affects what configurations can follow it)
        and how MANY times it is reached, this is much faster than
        considering every possible search path of configurations.

        We only need to keep the currently know configurations and
        their counts (in the cur_configs dictionary) and the possible
        next configurations found so far (in the new_configs
        dictionary) in storage. Since there can be a huge number of
        reachable configurations for larger grids, this is a major
        space savings.
    """

    cols = g.graph['cols']
    rows = g.graph['rows']

    cur_configs = {(0,)*cols:1}

    for row in range(rows):
        new_configs = defaultdict(int)
        target_degrees, next_neighbors = row_setup(g, row)

        for config, count in cur_configs.items():
            for new_config in next_configs(row, config, target_degrees, next_neighbors):
                new_configs[new_config] += count

        cur_configs = new_configs

    return sum(cur_configs.values())

def time(g, count=100):
    """ Benchmark the algorithm. """

    result = timeit.Timer(stmt=lambda:count_paths(g)).timeit(number=count)
    return "%s seconds for %s iterations, giving %s seconds per iteration" % (result, count, result/count)
    


if __name__ == "__main__":
    f = sys.stdin
    timing = None

    if len(sys.argv) > 1:
        f = open(sys.argv[1])

    if len(sys.argv) > 2:
        timing = int(sys.argv[2])

    g = read_graph(f)

    if timing:
        print time(g, timing)
    else:
        print count_paths(g)
    

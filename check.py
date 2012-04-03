from collections import defaultdict
from read_graph import read_graph
from itertools import count
from pprint import pprint


def check_path(g, path):
    #print "checking %s" % repr(path)
    if path[0] != g.graph['start_node']:
        return False, "Does not start at starting node %s" % repr(g.graph['start_node'])
    if path[-1] != g.graph['end_node']:
        return False, "Does not end at ending node %s" % repr(g.graph['end_node'])
    if len(path) != g.number_of_nodes():
        return False, "Not long enough to cover all nodes"
    if set(path) != set(g.nodes()):
        return False, "Does not cover all nodes"

    for u, v in ((path[idx-1], path[idx]) for idx in range(1, len(path))):
        if not g.has_edge(u, v):
            return False, "Edge %d->%d does not exist" % (u, v)

    return True, "No error"

def path_to_configs(g, path):
    g2 = g.copy()
    g2.clear()
    g2.add_path(path)

    rows = g.graph['rows']
    cols = g.graph['cols']


    def other_end(row, col):
        last = (row + 1, col)
        cur = (row, col)

        #print "--- %d" % col
        while True:
            next = [n2 for n2 in g2.neighbors(cur) 
                    if (n2 != last)]

            #print last, cur, next

            assert len(next) in (0,1)
            if len(next) == 0:
                return None
            next = next[0]

            if next[0] == row+1:
                return next[1]

            last, cur = cur, next

        
    configs = []
    configs.append((0,) * cols)
    for row in range(rows):
        line_ids = count(1)
        cur_config = []


        for col in range(cols):
            if g2.has_edge( (row, col), (row+1, col) ):
                other_col = other_end(row, col)
                if other_col is None or other_col > col:
                    cur_config.append(line_ids.next())
                else:
                    cur_config.append(cur_config[other_col])
            else:
                cur_config.append(0)


        configs.append(cur_config)
    return configs
                    


def print_path(g, path):
    rows = g.graph['rows']
    cols = g.graph['cols']
    g2 = g.copy()
    g2.clear()
    g2.add_path(path)

    for row in range(rows):
        print "".join("+%s" % ("-" if g2.has_edge( (row, col), (row, col+1)) else " ")
                      for col in range(cols))
        print "".join("%s " % ("|" if g2.has_edge( (row, col), (row+1, col)) else " ")
                      for col in range(cols))
    print
        

            

def iter_allpaths(g, filename):
    mapping = g.graph['numeric_id_to_node']
    for line in open(filename):
        line_no, _, path = line.partition(': ')
        line_no = int(line_no)
        path = [(int(x),int(y)) for (x,_,y) in 
                (pair.strip()[1:-1].partition(',') 
                 for pair in path.split(';'))]
        yield line_no, path

def check_allpaths(graph_filename, paths_filename):
    g = read_graph(graph_filename)
    for line_no, path in iter_allpaths(g, paths_filename):
        success, error = check_path(g, path)
        if success:
            continue
        print "%d: %s" % (line_no, error)


def configs_for_allpaths(graph_filename, paths_filename):
    g = read_graph(graph_filename)
    for line_no, path in iter_allpaths(g, paths_filename):
        print "%d: %r %r" % (line_no, path, path_to_configs(g, path))




def configs_per_row(paths_with_configs_filename):
    configs_by_row = defaultdict(set)
    for line in open(paths_with_configs_filename):
        line_no, _, rest = line.partition(":")
        rest = rest.strip()
        split = rest.index(" [")
        path = eval(rest[:split].strip())
        configs = eval(rest[split:].strip())
        print line_no
        
        for i, config in enumerate(configs):
            configs_by_row[i].add(tuple(configs[i]))

    for key in sorted(configs_by_row.keys()):
        items = configs_by_row[key]
        print "===================="
        print "configs generated for row %d" % key
        for config in sorted(list(items)):
            print config
        print "======================"
    

        

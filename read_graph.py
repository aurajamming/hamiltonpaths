import networkx as nx
from itertools import count

def read_graph(file_obj):
    entries = [int(x) for x in file_obj.read().split()]
    width, height = entries[0:2]

    # Build a full width x height grid graph

    def unit_dist(a, b):
        return (a[0]-b[0]) ** 2 + (a[1]-b[1]) ** 2 == 1

    nodes = [(row,col) 
             for col in range(width) 
             for row in range(height)]
    edges = [(a,b)
             for a in nodes
             for b in nodes
             if unit_dist(a, b)]

    g = nx.Graph(rows = height, cols = width)
    g.add_nodes_from(nodes)
    g.add_edges_from(edges)

    # mark graph with details and remove blocked nodes

    for index, code in enumerate(entries[2:]):
        node = divmod(index, width)

        g.node[node].update(code = code)

        if code == 0:            
            g.node[node].update(target_degree = 2)
        elif code == 1:
            g.remove_node(node)
        elif code == 2:
            g.node[node].update(target_degree = 1)
            g.graph.update(start_node = node)
        elif code == 3:
            g.node[node].update(target_degree = 1)
            g.graph.update(end_node = node)
        else:
            raise Exception("%d is not one of the node codes, at %s." % (code, node))

    return g

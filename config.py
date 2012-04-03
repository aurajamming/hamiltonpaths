from itertools import chain, count

"""
A Config(uration) represents the connectivity of a set of subpaths at a
given row in the graph. Only the connectivity is represented; the
series of subpaths (or other configurations) leading to a given
configuration is discarded.

A configuration is a list containing, for each column of the grid
graph at the given row, a representation of how the vertical edges
proceeding to next row are connected. The possible cases are:

  * No connection
  * A subpath that, in the rows above this configuration, connects to
    another vertical edge of this configuration. This is called a
    paired path.
  * A subpath that does not connect to any other subpath in this
    configuration. These are the unpaired subpaths.

A configuration for one row is converted to a configuration for the
next by considering each possible continuation of the paths it
represents (in the next_configs function) and recording connectivity
information by calling the link() and mask() methods of this class:

  * link(col_a, col_b) records that a new horizontal subpath connects
    the two given columns (col_a <= col_b). Linking two different
    subpaths together in this way causes the two subpaths to become a
    single subpath in the new configuration. The case of col_a ==
    col_b represents the spawning or closing of an unpaired path.

  * mask(mask) takes a list with elements 0 or 1 for each column. The
    configuration is adjusted to reflect that only those columns with
    a 1 the mask at their position have vertical edges that connect to
    the next row.

A canonical, human-readable display format is implemented on the
as_tuple method, in which each column is represented as a nonnegative
integer. A 0 indicates no connection; positive numbers represent
paths. If a nonzero number appears once, it's column is reached by an
otherwise unconnected subpath. Otherwise, nonzero numbers appear twice
in the list, once at each column position that the subpath connects.

An even-shorter, even-more-readable format is provided as the string
format (__str__). This returns a string with one digit per column,
with the same rules as the normal display format. It has the
limitation that it only works well for 9 or fewer subpaths.

The display format is canonical - there is only one display
representation for any unique configuration. This is enforced by
requiring sequential numbering for the paths, starting at 1 and
increasing by one for each new subpath not already seen while
considering the configuration from left to right.

This display format is human readable, compact, and can be stored as
dictionary keys. It is, not suitable to rapid manipulation. (It would
be possible to make Config instances implement __hash__ and __eq__ to
use them as keys; but the size of the representation used as keys
should be minimized as there will be a massive number of them used
during count_paths.)

Internally the connectivity information is represented by a list
mapping column numbers to "end records" stored in the "config"
member. An end record is either None (representing no connection), or
a two element list containing the column numbers of each end of the
subpath. For unpaired subpaths, the two end columns are the same.

If two columns are connected by a subpath, then the end stored for
those two columns is the same object, allowing comparison by identity,
modification in-place, and reducing the need for new list object
allocation to the case of spawning a new subpath.

"""


class Config(object):
    def __init__(self, initial_config, _copy_config=None, _copy_ends=None):
        if _copy_config is None:
            initial_config = [int(x) for x in initial_config]
            self.ends, self.config = Config.build_config(initial_config)
        else:
            self.ends, self.config = _copy_ends, _copy_config

        assert self.sanity_check()


    def copy(self):
        new_ends = [end[:] for end in self.ends]
        new_config = [None] * len(self)
        for end in new_ends:
            new_config[end[0]] = new_config[end[1]] = end

        assert all((new_config[c] is None) or (new_config[c] == self.config[c]) 
                   for c in range(len(self)))                   
        assert all((new_config[c] is None) or (new_config[c] is not self.config[c])
                   for c in range(len(self)))


        c = Config(None, _copy_config=new_config, _copy_ends=new_ends)

        assert c.sanity_check()
        return c
        

    def __len__(self):
        return len(self.config)

    def sanity_check(self):
        # print "SANITY_CHECK"
        # print "ends are: %s" % [(end, id(end)) for end in self.ends]
        # print "config is: %s" % repr(self.config)

        end_ids_seen = set()
        config = self.config
        for end in config:
            if end is not None:
                end_ids_seen.add(id(end))
            assert end is None or config[end[0]] is end
            assert end is None or config[end[1]] is end
            assert end is None or config[end[0]] is config[end[1]]

        # print "end_ids_seen is: %s" % repr(end_ids_seen)

        assert len(end_ids_seen) == len(self.ends)
        assert end_ids_seen == set(id(x) for x in self.ends)
        #assert len(ends) <= (len(config) // 2) + 1 # not quite true

        return True
            
            

    @staticmethod
    def path_pairs(config_seq):
        """ Given a sequence (list, string, etc) that is the display
            representation of a configuation, return a list of pairs
            of column numbers connected by a paired subpath and a list
            of column numbers occupied by an unpaired subpath.
        """
        seen, paired, lone = set(), [], []
        for col, line_id in enumerate(config_seq):
            if line_id == 0: 
                continue
            try:
                other = config_seq.index(line_id, col+1)
                paired.append( (col, other) )
            except:
                if line_id not in seen:
                    lone.append(col)
            seen.add(line_id)
        return paired, lone

    @staticmethod
    def build_config(config_seq):
        """ Given a display representation sequence, return an
            internal list-of-ends representation.
        """

        pairs, loners = Config.path_pairs(config_seq)
        config = [None] * len(config_seq)
        ends = []

        for col_a, col_b in chain(pairs, ((c,c) for c in loners)):
            assert col_a <= col_b
            end = [col_a, col_b]
            ends.append(end)
            config[col_a] = config[col_b] = end

        return ends, config


    def link(self, col_a, col_b):
        # print "LINK: %s, %s" % (col_a, col_b)
        assert self.sanity_check()
        assert col_a <= col_b

        config = self.config
        end_a = config[col_a]
        end_b = config[col_b]

        def adjust_path(end, from_col, to_col):
            cur_a, cur_b = end
            new_a = to_col if cur_a == from_col else cur_a
            new_b = to_col if cur_b == from_col else cur_b
            end[0], end[1] = (new_a, new_b) if new_a < new_b else (new_b, new_a)
            config[col_a], config[col_b] = config[col_b], config[col_a]

        def merge(end_a, end_b):
            new_a = end_a[1] if end_a[0] == col_a else end_a[0]
            new_b = end_b[1] if end_b[0] == col_b else end_b[0]

            if end_a[0] == end_a[1]:
                new_a = new_b
            if end_b[0] == end_b[1]:
                new_b = new_a
            
            end_a[0], end_a[1] = new_a, new_b
            config[new_a] = config[new_b] = end_a
            config[col_a] = config[col_b] = None
            self.ends.remove(end_b)
            if config[end_a[0]] == config[end_a[1]] == None:
                self.ends.remove(end_a)

        def split():
            # construct new end: this is the only link action that
            # should result in a new list object being allocated.
            new_end = [col_a, col_b]
            config[col_a] = config[col_b] = new_end
            self.ends.append(new_end)
            
        def close():
            self.ends.remove(end_a) # end_a == end_b here
            config[col_a] = config[col_b] = None

        if end_a is None and end_b is None:
            split()
        elif col_a == col_b:
            pass
        elif end_a is end_b:
            close()
        elif end_a is None:
            adjust_path(end_b, col_b, col_a)
        elif end_b is None:
            adjust_path(end_a, col_a, col_b)
        else:
            merge(end_a, end_b)

        assert self.sanity_check()

    def mask(self, vmask):
        # print "MASK: %s" % repr(vmask)
        assert self.sanity_check()
        assert len(vmask) == len(self.config)
        newconfig = [x if vmask[col] else None
                     for col, x in enumerate(self.config)]

        # clean up ends pointing to now-disconnected columns
        for end in self.ends:
            masked = [not vmask[end[x]] for x in (0,1)]
            if masked[0]:
                if masked[1]:
                    self.ends.remove(end)
                else:
                    end[0] = end[1]
            elif masked[1]:
                end[1] = end[0]
        
        self.config = newconfig

        assert self.sanity_check()

    def would_close(self, col_a, col_b):
        """ Return True if calling link(col_a, col_b) would result in
            a subpath being closed, False otherwise. Closing a paired
            subpath would result in a cycle, which are disallowed in a
            hamiltonian path.
        """

        assert self.sanity_check()
        assert col_a < col_b

        config = self.config
        end_a, end_b = config[col_a], config[col_b]

        assert self.sanity_check()
        
        return (end_a is not None and
                end_b is not None
                and end_a is end_b)

    def __str__(self):
        # DEFECT: if there are more than 9 subpaths, this will return
        # nonsense since there are only 9 single-character decimal
        # digits
        # TODO: use hex to extend this to 15
        # TODO: a solution for the > 15 case
        return "".join(str(x) for x in self.as_tuple())

    def __repr__(self):
        # Some useful debugging data: standard string representation,
        # along with the contents and object identity of each column's
        # ends.
        out = ", ".join((str(x)+" (%x)" % id(x)) if x else "" for x in self.config )
        return "%s <%s>" % (str(self), out)

    def as_tuple(self):
        # Return a tuple of numbered paths, possibly as a first step
        # to producing a display representation.
        ids = count(1)
        numbering = {}
        for end in [tuple(x) for x in self.config if x]:
            if end not in numbering:
                numbering[end] = ids.next()
        return  tuple(numbering[tuple(end)] if end else 0
                      for end in self.config)
            

def test():            

    tests = [
        ("2332", [(2,3)], "1100"),
        ("120201", [(1,2),(3,5)], "101000"),
        ("1002332", [(0,2),(5,6)], "0012200"),
        ("12233", [(2,3)], "12002"),
        ("0000", [(1,2)], "0110"),
        ("0000", [(0,1),(2,3)], "1122"),
        ("1221", [(1,2)], "1001"),
        ("100", [(0,0)], "100"),
        ("000", [(0,0)], "100"),
        ("010", [(1,1)], "010"),
        ("000", [(1,1)], "010"),
        ("01202", [(0,1), "10101", (2,3), "10011"], "10022"),
        ("10220", [(0,1), (3,4)], "01202"),
        ("1234432", [(2,3), (5,6)], "1200200"),
        ("1202", [(0,1)], "0001"),
    ]


    for a, l, c in tests:
        print "%s  via %s to %s:" % (a, l, c)
        x = Config(a)
        print "  %s" % repr(x)
        for action in l:
            if isinstance(action, tuple):
                print "  linked across %s to" % str(action)
                x.link(*action)
            else:
                print "  masked with %s to" % str(action)
                x.mask([int(m) for m in action])
            print "  %s" % repr(x)

        if str(x) == c:
            print "ok, came to %s" % x
        else:
            print "** FAILED **, was %s should be %s" % (x, c)



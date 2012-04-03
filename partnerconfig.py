from itertools import chain, count

class PartnerConfig(object):
    def __init__(self, initial_config, _copy_config=None):
        if _copy_config is None:
            initial_config = [int(x) for x in initial_config]
            self.config = PartnerConfig.build_config(initial_config)
        else:
            self.config = _copy_config

        assert self.sanity_check()


    def copy(self):
        c = PartnerConfig(None, _copy_config=self.config[:])

        assert c.sanity_check()
        return c
        

    def __len__(self):
        return len(self.config)

    def sanity_check(self):
        config = self.config
        for i in range(len(config)):
            if config[i] == -1:
                continue
            assert config[config[i]] == i

        return True
            
            

    @staticmethod
    def path_pairs(config_seq):
        """ Given a sequence (list, string, etc) that is the display
            representation of a configuation, return a list of pairs
            of column numbers connected by a paired subpath and a list
            of column numbers occupied by an unpaired subpath.
        """
        seen = set()
        for col, line_id in enumerate(config_seq):
            if line_id == 0: 
                continue
            try:
                other = config_seq.index(line_id, col+1)
                yield (col, other)
            except:
                if line_id not in seen:
                    yield (col, col)
            seen.add(line_id)

    @staticmethod
    def build_config(config_seq):
        """ Given a display representation sequence, return an
            internal list-of-ends representation.
        """

        config = [-1] * len(config_seq)
        for col_a, col_b in PartnerConfig.path_pairs(config_seq):
            assert col_a <= col_b
            config[col_a] = col_b
            config[col_b] = col_a

        return config


    def link(self, col_a, col_b):
        # print "LINK: %s, %s" % (col_a, col_b)
        assert self.sanity_check()
        assert col_a <= col_b

        def adjust_path(partner, col_from, col_to):
            config[col_from] = -1
            if partner == col_from:
                config[col_to] = col_to
            else:
                config[partner] = col_to
                config[col_to] = partner


        def merge():
            config[partner_a] = partner_b
            config[partner_b] = partner_a
            config[col_a] = config[col_b] = -1
            if partner_a == col_a:
                config[partner_b] = partner_b
            elif partner_b == col_b:
                config[partner_a] = partner_a

        def split():
            config[col_a] = col_b
            config[col_b] = col_a
            
        def close():
            config[col_a] = config[col_b] = -1

        config = self.config
        partner_a = config[col_a]
        partner_b = config[col_b]

        if partner_a == partner_b == -1:
            split()
        elif col_a == col_b:
            pass
        elif col_a == partner_b: # and col_b == partner_a:  (only one test needed here)
            close()
        elif partner_a == -1:
            adjust_path(partner_b, col_b, col_a)
        elif partner_b == -1:
            adjust_path(partner_a, col_a, col_b)
        else:
            merge()

        assert self.sanity_check()

    def mask(self, vmask):
        assert self.sanity_check()
        assert len(vmask) == len(self.config)

        newconfig = self.config[:]
        for col in range(len(newconfig)):
            if col == -1 or vmask[col]:
                continue

            partner = newconfig[col]
            newconfig[col] = -1
            if partner >= 0 and partner != col:
                newconfig[partner] = partner

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

        partner_b = self.config[col_b]
        return col_a == partner_b
                

    def __str__(self):
        # DEFECT: if there are more than 9 subpaths, this will return
        # nonsense since there are only 9 single-character decimal
        # digits
        # TODO: use hex to extend this to 15
        return "".join(str(x) for x in self.path_ids())

    def __repr__(self):
        return "%s <%s>" % (str(self), "".join(str(d) if d >= 0 else "x" 
                                               for d in self.config))

    def path_ids(self):
        ids = count(1)
        path_ids = [0] * len(self.config)
        for col in range(len(self.config)):
            partner = self.config[col]
            if partner == -1:
                continue
            if partner < col:
                path_ids[col] = path_ids[partner]
            else:
                path_ids[col] = next(ids)
        return path_ids

    


            

def test():            

    tests = [
        ("1221", [(2,3)], "1100"),
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
        x = PartnerConfig(a)
        print "  %s" % repr(x)
        assert(str(x) == a)
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



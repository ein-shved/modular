
def printer(out):
    if callable(out):
        def p(in_str=None):
            nonlocal out
            if in_str == None:
                return out
            return out(in_str)
        return p
    elif isinstance(out, str):
        def p(in_str = None):
            nonlocal out
            if in_str == None:
                return out
            out = out + in_str + '\n'
            return out
        return p

def increase_printer(out):
    if callable(out):
        def p(in_str=None):
            nonlocal out
            if in_str == None:
                return out
            if not in_str:
                return out(in_str)
            return out('    ' + in_str)
        return p
    elif isinstance(out, str):
        def p(in_str = None):
            nonlocal out
            if in_str == None:
                return out
            if not in_str:
                out += '\n'
                return out
            out = out + '    ' + in_str + '\n'
            return out
        return p


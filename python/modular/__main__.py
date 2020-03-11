from .module import Module
from .version import VERSION
from yaml import CLoader as Loader, load
from os import path, getenv, makedirs
from argparse import ArgumentParser

class InvalidModulesVersionException(Exception):
    pass

class InvalidModulesSyntaxException(Exception):
    pass

def printer_pretty(name=None):
    if name:
        print ("Generating " + name),

def printer_python(name=None):
    if printer_python.first:
        print ('[', end='')
        printer_python.first=False
    elif name:
        print(', ', end='')

    if name:
        print ('"' + str(name) + '"', end='')
    else:
        print (']')
printer_python.first=True

def printer_cmake(name=None):
    if not printer_cmake.first and name:
        print (';', end='')
    printer_cmake.first=False
    if name:
        print (str(name), end='')
    else:
        print('')
printer_cmake.first=True

printers = {
    'pretty'    : printer_pretty,
    'python'    : printer_python,
    'cmake'     : printer_cmake,
}

if __name__ == "__main__":
    parser = ArgumentParser("The C code generator for modular protocol")
    parser.add_argument('module', help="the yaml file with module description")
    parser.add_argument('--out', '-o',
                        help="output path, relative to builddir or absolute, " +
                             "defaults to the module file relative path",
                        default=None)
    parser.add_argument('--builddir', '-b',
                        help="base output directory, defaults to env " +
                             "BUILDDIR, or current path",
                        default=getenv('BUILDDIR', '.'))
    parser.add_argument('--list', '-l',
                        help="just list output files names, no actually run",
                        action='store_true')
    parser.add_argument('--format', '-f',
                        help="set format of list of generated files",
                        choices=printers.keys(),
                        default='pretty')
    parser.add_argument('--absolute', '-a',
                        help="with this option print absolute path, without " +
                        "- relative to builddir",
                        action='store_true')
    args = parser.parse_args()
    y = open(args.module, "r").read()
    modules = load(y, Loader)
    bd = path.abspath(args.builddir)
    if args.out:
        if path.isabs(args.out):
            bd = args.out
        else:
            bd = path.join(bd, args.out)
    else:
        cp = path.commonprefix([args.module, bd])
        rel = path.relpath(args.module, cp)
        bd = path.join (bd, path.dirname(rel))
    if 'version' not in modules or str(modules['version']) != VERSION:
        raise InvalidModulesVersionException()

    if 'modules' not in modules:
        raise InvalidModulesSyntaxException()

    printer=printers[args.format]
    for n, m in modules['modules'].items():
        makedirs(bd, exist_ok=True)
        module = Module(n, m, args.module)
        fname=path.join(bd, module.h_name())
        printer(fname if args.absolute else path.relpath(fname, args.builddir))
        if not args.list:
            h = module.put_h()
            hf = open(fname, "w")
            hf.write(h)
            hf.close()

        fname=path.join(bd, module.c_name())
        printer(fname if args.absolute else path.relpath(fname, args.builddir))
        if not args.list:
            c = module.put_c()
            cf = open(fname, "w")
            cf.write(c)
            cf.close()
    printer()

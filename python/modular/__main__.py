from .module import Module
from .version import VERSION
from yaml import CLoader as Loader, load
from os import path, getenv, makedirs
from argparse import ArgumentParser

class InvalidModulesVersionException(Exception):
    pass

class InvalidModulesSyntaxException(Exception):
    pass

if __name__ == "__main__":
    parser = ArgumentParser("The C code generator for modular protocol")
    parser.add_argument('module', help="the yaml file with module description")
    parser.add_argument('--out', '-o',
                        help="output path, relative to builddir, defaults to " +
                             "the module file relative path",
                        default=None)
    parser.add_argument('--builddir', '-b',
                        help="base output directory, defaults to env " +
                             "BUILDDIR, or current path",
                        default=getenv('BUILDDIR', '.'))
    args = parser.parse_args()
    y = open(args.module, "r").read()
    modules = load(y, Loader)
    bd = args.builddir + '/'
    if args.out:
        bd += args.out
    else:
        bd += path.dirname(args.module)
    if 'version' not in modules or str(modules['version']) != VERSION:
        raise InvalidModulesVersionException()

    if 'modules' not in modules:
        raise InvalidModulesSyntaxException()

    for n, m in modules['modules'].items():
        makedirs(bd, exist_ok=True)
        module = Module(n, m, args.module)
        print ("Generating " + module.h_name())
        h = module.put_h()
        hf = open(bd + '/' + module.h_name(), "w")
        hf.write(h)
        hf.close()

        print ("Generating " + module.c_name())
        c = module.put_c()
        cf = open(bd + '/' + module.c_name(), "w")
        cf.write(c)
        cf.close()

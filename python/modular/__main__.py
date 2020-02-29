from .module import Module
from .version import VERSION
from yaml import CLoader as Loader, load
from os import path
import sys

class InvalidModulesVersionException(Exception):
    pass

class InvalidModulesSyntaxException(Exception):
    pass

if __name__ == "__main__":
    filename = sys.argv[1]
    y = open(sys.argv[1], "r").read()
    modules = load(y, Loader)
    bd = path.dirname(sys.argv[1])
    if 'version' not in modules or str(modules['version']) != VERSION:
        raise InvalidModulesVersionException()

    if 'modules' not in modules:
        raise InvalidModulesSyntaxException()

    for n, m in modules['modules'].items():
        module = Module(n, m, filename)
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

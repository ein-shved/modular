
from .printer import increase_printer
from .struct import Struct
from .argument import Argument

class Method(Struct):
    def __init__ (self, module, name, num, obj):
        self._module = module
        self._name = name.lower()
        self._num = num
        self._arguments = []
        self._receive = "method_" + self.name() + "_receive";
        if isinstance(obj, list):
            for arg in obj:
                self._arguments.append(arg.upper())
        if isinstance(obj, dict):
            if 'arguments' in obj:
                for arg in obj['arguments']:
                    if isinstance(obj['arguments'], dict):
                        darg = Argument(arg.upper(),
                            default=obj['arguments'][arg])
                    else:
                        darg = Argument(arg.upper())
                    self._arguments.append(darg)
            if 'receive' in obj:
                self._receive = obj["receive"]
        Struct.__init__(self, "method_{m}_{n}_dsc".format(m=module.name(),
                                                          n=self.name()))
    def name(self):
        return self._name
    def uppername(self):
        return self._name.upper()
    def enum_name(self):
        return 'EV_' + self._module.uppername() + '_' + self.uppername()
    def size(self):
        pass
    def put_fields(self, pr):
        for arg in self._arguments:
            pr("arg_name_t {n}; /* {V} */".format(n=arg.lower(), V=arg));
    def put_receiver(self, pr):
        line = None
        for arg in self._arguments:
            if line == None:
                line = "void {n} (".format(n=self._receive)
            else:
                line += ", "
            line += "uint8_t {n}".format(n=arg)
        line += ", uint16_t vac, uint8_t vav[]"
        line += ");"
        pr(line)

    def put_vals_in_module(self, pr):
        pr('.method_{n} = {{'.format(n=self.name()))
        self.put_args_in_module(increase_printer(pr))
        pr('},')

    def put_args_in_module(self, pr):
        for arg in self._arguments:
            pr(".{n} = {{ '{u[0]}', '{u[1]}', '{u[2]}', }},".format(
                    n=arg.lower(), u=arg))
    def get_call(self, *args):
        line = None;
        for i in range(0, len(self._arguments)):
            if i in args:
                arg = args[i]
            else:
                arg = self._arguments[i].default()
            if line:
                line = '{n}('.format(n=self._receive)
            else:
                line += ', '
            line += str(arg)
        return line + ')'

    def call_format(self, **kwargs):
        predicate_format=kwargs['predicate']
        argument_formart=kwargs['argument']
        default_format = None
        pr = None
        va = 'NULL'
        valen = '0'
        line = '{n}('.format(n=self._receive)
        if 'default' in kwargs:
            default_format = kwargs['default']
        if 'pr' in kwargs:
            pr = kwargs['pr']
            ipr = increase_printer(pr)
            pr(line)
        if 'va' in kwargs and 'valen' in kwargs:
            va = kwargs['va']
            valen = kwargs['valen']
        elif 'va_fmt' in kwargs and 'valen_fmt' in kwargs:
            va = "{test} ? {arg} : NULL".format(
                test = predicate_format.format(i = len(self._arguments)),
                arg = kwargs['va_fmt'].format(i = len(self._arguments)))
            valen = "{test} ? {arg} : 0".format(
                test = predicate_format.format(i = len(self._arguments)),
                arg = kwargs['valen_fmt'].format(i = len(self._arguments)))
        if 'tail' in kwargs:
            tail = ")" + kwargs['tail']
        else:
            tail = ")"

        for i in range(0, len(self._arguments)):
            test=predicate_format.format(i=i)
            argument=argument_formart.format(i=i)
            if default_format:
                default=default_format.format(i=i)
            else:
                default=self._arguments[i].default()
            arg="({test}) ? ({argument}) : ({default})".format(
                 test=test, argument=argument, default=default)
            arg += ','
            if pr:
                ipr(arg)
            else:
                line += arg
        if pr:
            ipr(valen + ",")
            ipr(va + tail)
        else:
            line += valen + ", " + va + tail
        if pr:
            return pr()
        else:
            return line

    def put_call(self, pr, *args):
        pr(self.get_call(*args) + ';')


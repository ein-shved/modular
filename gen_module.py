import yaml
from yaml import CLoader as Loader
import sys

VERSION = '0.1'

class InvalidModulesVersionException(Exception):
    pass

class InvalidModulesSyntaxException(Exception):
    pass


def printer(out):
    if callable(out):
        def p(in_str):
            nonlocal out
            return out(in_str)
        return p
    elif isinstance(out, str):
        def p(in_str):
            nonlocal out
            if not in_str and in_str != "":
                return out
            out = out + in_str + '\n'
            return out
        return p
def increase_printer(out):
    if callable(out):
        def p(in_str):
            nonlocal out
            return out('    ' + in_str)
        return p
    elif isinstance(out, str):
        def p(in_str):
            nonlocal out
            if not in_str:
                return out
            out = out + '    ' + in_str + '\n'
            return out
        return p

class Struct:
    class InvalidNamingCombinationException(Exception):
        pass
    class InvalidPredefinitionRequestException(Exception):
        pass
    class InvalidVariableRequestException(Exception):
        pass
    def __init__(self, name, with_struct=True, with_typedef=False):
        self._s_name = name
        self._with_struct = with_struct
        self._with_typedef = with_typedef
    def get_type(self):
        return "{n}_t".format(n=self._s_name)
    def get_struct(self):
        return "struct {n}".format(n=self._s_name)
    def put_predefinition(self, pr):
        if not self._with_struct:
            raise InvalidPredefinitionRequestException()
        elif self._with_typedef:
            pr("typedef {s} {n};".format(s=self.get_struct(),
                                         n=self.get_type()))
        else:
            pr("{s};".format(s=self.get_struct()))
    def put_definition(self, pr, variable=None):
        first_line=""
        last_line=""
        if not self._with_struct and not self._with_typedef and not variable:
            raise InvalidNamingCombinationException();
        if self._with_typedef:
            first_line += "typedef "
            last_line = "}} STRUCT_PACKED {n}".format(n=self.get_type())
        else:
            last_line = "} STRUCT_PACKED"
        if self._with_struct:
            first_line += "{s} {{".format(s=self.get_struct())
        else:
            first_line += "struct {"
        if variable:
            last_line += " " + variable
        last_line += ";"
        pr(first_line);
        self.put_fields(increase_printer(pr))
        pr(last_line)

    def put_variable(self, pr, variable, value=None):
        if not self._with_struct and not self._with_typedef:
            raise InvalidVariableRequestException()
        line = ""
        if self._with_typedef:
            line = self.get_type()
        else:
            line = self.get_struct()
        line += " {v}".format(v=variable)
        if value:
            line += " = {v}".value
        line += ";"
        pr(line)

class Event (Struct):
    def __init__ (self, module, name, num, obj):
        self._module = module
        self._name = name.lower()
        self._num = num
        self._arguments = []
        self._send = 'event_{n}_send'.format (n=self.name())
        if isinstance(obj, list):
            for arg in obj:
                self._arguments.append(arg.upper())
        if isinstance(obj, dict) and 'arguments' in obj:
            for arg in obj['arguments']:
                self._arguments.append(arg.upper())
        Struct.__init__(self, "event_{m}_{n}_dsc".format(m=module.name(),
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

    def put_sender(self, pr):
        line = None
        for arg in self._arguments:
            if line == None:
                line = "static inline int {n} (".format(n=self._send)
            else:
                line += ", "
            line += "uint8_t {n}".format(n=arg)
        line += ");"
        pr(line)

    def put_sender_definition(self, pr):
        line = None
        for arg in self._arguments:
            if line == None:
                line = "static inline int {n} (".format(n=self._send)
            else:
                line += ", "
            line += "uint8_t {n}".format(n=arg.lower())
        line +=")"
        pr(line)
        pr("{")
        self.put_sender_body(increase_printer(pr))
        pr("}")

    def put_sender_body(self, pr):
        ipr = increase_printer(pr)
        n = self.uppername()
        pr("uint8_t data [] = {")
        ipr("'{n0}', '{n1}', '{n2}', // name".format(n0=n[0], n1=n[1], n2=n[2]))
        ipr('{num}, // arg num'.format(num=len(self._arguments)))
        for arg in self._arguments:
            ipr ("{n},".format(n = arg.lower()))
        pr("};")
        pr("return send_event (data);")

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
                    self._arguments.append(arg.upper())
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
        line += ");"
        pr(line)

class Module (Struct):
    def __init__ (self, name, obj):
        self._name = name.lower()
        self._events = []
        self._methods = []
        self._sender = name + '_send_data'

        if 'events' in obj:
            for n, e in obj['events'].items():
                self._events.append(Event(self, n, len(self._events), e))
        if 'methods' in obj:
            for n, m in obj['methods'].items():
                self._methods.append(Method(self, n, len(self._methods), m))
        if 'sender' in obj:
            self._sender = obj['sender']
        Struct.__init__(self, "module_{n}_dsc".format(n=self.name()),
                                                      True, True)

    def name(self):
        return self._name
    def uppername(self):
        return self._name.upper()
    def h_name(self):
        return self.name() + '.h'
    def c_name(self):
        return self.name() + '.c'
    def put_h(self):
        out = ""
        pr = printer(out)
        pr('#ifndef {}_H'.format(self.uppername()))
        pr('#define {}_H'.format(self.uppername()))
        pr('#include "modular.h"')
        pr('')
        self.put_methods_predefinitions(pr)
        pr("");
        self.put_events_predefinitions(pr)
        pr("");
        self.put_definition(pr)
        pr("");
        self.put_methods_receivers(pr)
        pr("");
        self.put_events_senders(pr)
        pr("");
        self.put_methods_definitions(pr)
        pr("");
        self.put_events_definitions(pr)
        pr("");
        self.put_events_senders_definitions(pr)
        pr("");
        return pr('#endif /* {}_H */'.format(self.uppername()))

    def put_methods_predefinitions(self, pr):
        for m in self._methods:
            m.put_predefinition(pr)

    def put_methods_definitions(self, pr):
        for m in self._methods:
            m.put_definition(pr)
            pr("")

    def put_methods_receivers(self, pr):
        for m in self._methods:
            m.put_receiver(pr)
        pr("")

    def put_events_predefinitions(self, pr):
        for e in self._events:
            e.put_predefinition(pr)

    def put_events_definitions(self, pr):
        for e in self._events:
            e.put_definition(pr)
            pr("")

    def put_events_senders(self, pr):
        for e in self._events:
            e.put_sender(pr);

    def put_events_senders_definitions(self, pr):
        for e in self._events:
            e.put_sender_definition(pr);

    def put_fields(self, pr):
        pr('module_dsc_t base;')
        pr("")
        for m in self._methods:
            m.put_variable(pr, "method_{n}".format(n=m.name()))
        pr("")
        for e in self._events:
            e.put_variable(pr, "event_{n}".format(n=e.name()))
    def put_c(self):
        out = ""
        pr = printer(out)
        return pr("#include \"{}\"\n".format(self.h_name()))

y = open(sys.argv[1], "r").read()
modules = yaml.load(y, Loader)

print(modules)

if 'version' not in modules or str(modules['version']) != VERSION:
    raise InvalidModulesVersionException()

if 'modules' not in modules:
    raise InvalidModulesSyntaxException()

for n, m in modules['modules'].items():
    module = Module(n, m)
    print ("Generating " + module.h_name())
    hf = open(module.h_name(), "w")
    hf.write(module.put_h())
    hf.close()

    print ("Generating " + module.c_name())
    cf = open(module.c_name(), "w")
    cf.write(module.put_c())
    cf.close()

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
            return out('    ' + in_str)
        return p
    elif isinstance(out, str):
        def p(in_str = None):
            nonlocal out
            if in_str == None:
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

    def get_variable(self, variable, value=None):
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
        return line

    def put_variable(self, pr, variable, value=None):
        pr(self.get_variable(variable, value) + ";")

class Argument (str):
    def __init__ (self, *args, **kwargs):
        self._default = 0
        str(*args, **kwargs)
        if 'default' in kwargs:
            self._default = kwargs['default']
    def default(self):
        return self._default

class Event (Struct):
    def __init__ (self, module, name, num, obj, inline=False):
        self._module = module
        self._name = name.lower()
        self._num = num
        self._arguments = []
        self._send = 'event_{n}_send'.format (n=self.name())
        self._inline = inline
        if isinstance(obj, list):
            for arg in obj:
                self._arguments.append(arg.upper())
        if isinstance(obj, dict) :
            if 'arguments' in obj:
                for arg in obj['arguments']:
                    if isinstance(obj['arguments'], dict):
                        darg = Argument(arg.upper(),
                            default=obj['arguments'][arg])
                    else:
                        darg = Argument(arg.upper())
                    self._arguments.append(darg)
            if 'inline' in obj:
                self._inline = obj['inline']
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
    def is_inline(self):
        return self._inline

    def get_sender_prototype(self):
        line = None
        for arg in self._arguments:
            if line == None:
                line = "{i}int {n} ({m}, ".format(n=self._send,
                    m='module_id_t id',
                    i=self.is_inline() and "static inline " or "")
            else:
                line += ", "
            line += "uint8_t {n}".format(n=arg.lower())
        return line + ")"

    def put_sender(self, pr):
        pr(self.get_sender_prototype() + ";")

    def put_sender_definition(self, pr):
        pr(self.get_sender_prototype())
        pr("{")
        self.put_sender_body(increase_printer(pr))
        pr("}")

    def put_sender_body(self, pr):
        ipr = increase_printer(pr)
        iipr = increase_printer(ipr)
        n = self.uppername()
        pr("event_t event = {")
        ipr(".msg = {")
        self._module.put_message(iipr, 'MSG_EVENT', 'id',
                                 len(self._arguments) + 1)
        ipr("},")
        ipr(".event_id = {id},".format(id=self._num))
        ipr(".args = {")
        for arg in self._arguments:
            iipr ("{n},".format(n = arg.lower()))
        ipr("},");
        pr("};")
        pr("return {s};".format(s=self._module.send('&event.msg')))

    def put_vals_in_module(self, pr):
        pr('.event_{n} = {{'.format(n=self.name()))
        self.put_args_in_module(increase_printer(pr))
        pr('},')

    def put_args_in_module(self, pr):
        for arg in self._arguments:
            pr(".{n} = {{ '{u[0]}', '{u[1]}', '{u[2]}', }},".format(
                n=arg.lower(), u=arg))

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
        line = '{n}('.format(n=self._receive)
        if 'default' in kwargs:
            default_format = kwargs['default']
        if 'pr' in kwargs:
            pr = kwargs['pr']
            ipr = increase_printer(pr)
            pr(line)
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
            if i < len(self._arguments) - 1:
                arg += ','
            else:
                arg += tail
            if pr:
                ipr(arg)
            else:
                line += arg
        if pr:
            return pr()
        else:
            return line

    def put_call(self, pr, *args):
        pr(self.get_call(*args) + ';')

class Module (Struct):
    def __init__ (self, name, obj):
        self._name = name.lower()
        self._events = []
        self._methods = []
        self._sender = name + '_send_data'
        events_inline = False

        if 'events_inline' in obj:
            events_inline = obj['events_inline']
        if 'events' in obj:
            for n, e in obj['events'].items():
                self._events.append(Event(self, n, len(self._events) + 1, e,
                    events_inline))
        if 'methods' in obj:
            for n, m in obj['methods'].items():
                self._methods.append(Method(self, n, len(self._methods) + 1, m))
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
        pr("")
        self.put_sender_prototype(pr)
        self.put_describer_prototype(pr)
        self.put_receiver_predefenition(pr)
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

    def put_events_senders_definitions(self, pr, h_file=True):
        for e in self._events:
            if e.is_inline() == h_file:
                e.put_sender_definition(pr);

    def put_fields(self, pr):
        pr('module_dsc_t base;')
        pr("")
        for m in self._methods:
            m.put_variable(pr, "method_{n}".format(n=m.name()))
        pr("")
        for e in self._events:
            e.put_variable(pr, "event_{n}".format(n=e.name()))

    def put_sender_prototype(self, pr):
        pr ("int {n}(message_t *msg);".format(n=self._sender))

    def put_c(self):
        out = ""
        pr = printer(out)
        pr("#include \"{}\"\n".format(self.h_name()))
        pr("")
        self.put_events_senders_definitions(pr, False)
        pr("")
        self.put_describer(pr)
        pr("")
        self.put_receiver(pr)
        return pr()

    def put_message(self, pr, msg, module_id, data_size):
        if isinstance(data_size, str):
            size = '(' + data_size + ' + 1)'
        else:
            size = data_size + 1
        pr (".msg = {t},".format(t=msg))
        pr (".id = {i},".format(i=module_id))
        pr (".size = hton16({s}),".format(s=size))

    def send(self, data):
        return '{s}({d})'.format(s=self._sender, d=data)

    def get_describer_prototype(self):
        return "int {n}_send_description(module_id_t id)".format(n=self._name)

    def put_describer_prototype(self, pr):
        pr(self.get_describer_prototype() + ';')

    def put_describer(self, pr):
        pr(self.get_describer_prototype())
        pr('{')
        self.put_describer_body(increase_printer(pr))
        pr('}')

    def put_describer_body(self, pr):
        ipr = increase_printer(pr)
        iipr = increase_printer(ipr)
        pr('static MODULE_T({n}) msg = {{'.format(n=self.get_type()))
        ipr('.msg = {')
        self.put_message(iipr, 'MSG_MODULE_DSC', 'id',
                         'sizeof({t})'.format(t=self.get_type()) )
        ipr('},')
        ipr('.module = {')
        self.put_module_vals(increase_printer(ipr))
        ipr('},')
        pr('};')
        pr('return {r};'.format(r=self.send('&msg.msg')))

    def put_module_vals(self, pr):
        pr('.base = {')
        self.put_base_module_vals(increase_printer(pr))
        pr ('},')
        for m in self._methods:
            m.put_vals_in_module(pr)
        for e in self._events:
            e.put_vals_in_module(pr)

    def put_base_module_vals(self, pr):
        pr(".type = {{ '{sn[0]}', '{sn[1]}', '{sn[2]}' }}, ".format(
                    sn=self._name.upper()))
        pr(".num_methods = {n},".format(n=len(self._methods)))
        pr(".num_events = {n},".format(n=len(self._events)))

    def get_receiver_prototype(self):
        return 'int {n}_process_message(uint8_t *data)'.format(n=self._name)

    def put_receiver_predefenition(self, pr):
        pr(self.get_receiver_prototype() + ';')

    def put_receiver(self, pr):
        self.put_method_receiver_predefenition(pr)
        self.put_event_receiver_predefenition(pr)
        pr('')
        pr(self.get_receiver_prototype())
        pr('{')
        self.put_receiver_body(increase_printer(pr))
        pr('}')
        self.put_method_receiver(pr)
        self.put_event_receiver(pr)

    def put_receiver_body(self, pr):
        ipr = increase_printer(pr)
        pr('message_t *msg = (message_t *)data')
        pr('')
        pr('switch(msg->msg) {')
        pr('case MSG_EVENT:')
        ipr('return handle_event(msg);')
        pr('case MSG_METHOD:')
        ipr('return handle_method(msg);')
        pr('default:')
        ipr('return EUNSUPPORTED;')
        pr('}')

    def get_event_receiver_prototype(self):
        return 'static int handle_event(message_t *msg)'

    def get_method_receiver_prototype(self):
        return 'static int handle_method(message_t *msg)'

    def put_event_receiver_predefenition(self, pr):
        return pr(self.get_event_receiver_prototype() + ';')

    def put_method_receiver_predefenition(self, pr):
        return pr(self.get_method_receiver_prototype() + ';')

    def put_event_receiver(self, pr):
        pr(self.get_event_receiver_prototype())
        pr('{')
        self.put_event_receiver_body(pr)
        pr('}')

    def put_method_receiver(self, pr):
        pr(self.get_method_receiver_prototype())
        pr('{')
        self.put_method_receiver_body(increase_printer(pr))
        pr('}')
    def put_event_receiver_body(self, pr):
        pass
    def put_method_receiver_body(self, pr):
        ipr = increase_printer(pr)
        pr('method_t *method = (method_t *)msg;')
        pr('')
        pr('switch(method->method_id) {')
        for m in self._methods:
            pr('case {n}:'.format(n=m._num))
            ipr('return')
            m.call_format(predicate="{i} < (msg->size - 6)",
                          argument="method->args[{i}]",
                          pr=increase_printer(ipr),
                          tail=';')
        pr('default:')
        ipr('return EINVALIDMETHOD;')


y = open(sys.argv[1], "r").read()
modules = yaml.load(y, Loader)

if 'version' not in modules or str(modules['version']) != VERSION:
    raise InvalidModulesVersionException()

if 'modules' not in modules:
    raise InvalidModulesSyntaxException()

for n, m in modules['modules'].items():
    module = Module(n, m)
    print ("Generating " + module.h_name())
    h = module.put_h()
    hf = open(module.h_name(), "w")
    hf.write(h)
    hf.close()

    print ("Generating " + module.c_name())
    c = module.put_c()
    cf = open(module.c_name(), "w")
    cf.write(c)
    cf.close()

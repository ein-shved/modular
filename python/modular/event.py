
from .printer import increase_printer
from .struct import Struct

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

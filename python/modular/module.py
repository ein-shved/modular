
from .struct import Struct
from .printer import printer, increase_printer
from .method import Method
from .event import Event

class Module (Struct):
    def __init__ (self, name, obj, filename):
        self._name = name.lower()
        self._events = []
        self._methods = []
        self._sender = name + '_send_data'
        self._event_table_get = name + '_event_table_get'
        self._filename = filename
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
        if 'event_table_get' in obj:
            self._event_table_get = obj['event_table_get']
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
        pr('''/*
* Automatically generated from {fname}.
* DO NOT EDIT THIS
*
* For more information see https://github.com/ein-shved/modular or
* contact Yury Shvedov <mestofel13@gmail.com>
*
*/'''.format(fname=self._filename))

        pr('')
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
        pr('''/*
* Automatically generated from {fname}.
* DO NOT EDIT THIS
*
* For more information see https://github.com/ein-shved/modular or
* contact Yury Shvedov <mestofel13@gmail.com>
*
*/'''.format(fname=self._filename))
        pr('')
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
        self.put_event_processor_predefenition(pr)
        pr('')
        pr(self.get_receiver_prototype())
        pr('{')
        self.put_receiver_body(increase_printer(pr))
        pr('}')
        self.put_method_receiver(pr)
        self.put_event_processor(pr)

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
        ipr('return -EUNSUPPORTED;')
        pr('}')

    def get_method_receiver_prototype(self):
        return 'static int handle_method(message_t *msg)'

    def get_event_processor_prototype(self):
        return 'int process_event(event_handler_t *handler, event_t *event)'

    def put_method_receiver_predefenition(self, pr):
        return pr(self.get_method_receiver_prototype() + ';')

    def put_event_processor_predefenition(self, pr):
        return pr(self.get_event_processor_prototype() + ';')

    def put_method_receiver(self, pr):
        pr(self.get_method_receiver_prototype())
        pr('{')
        self.put_method_receiver_body(increase_printer(pr))
        pr('}')

    def put_method_receiver_body(self, pr):
        ipr = increase_printer(pr)
        pr('method_t *method = (method_t *)msg;')
        pr('')
        pr('switch(method->method_id) {')
        for m in self._methods:
            pr('case {n}:'.format(n=m._num))
            ipr('return')
            m.call_format(predicate="{i} < MT_NUM_ARGS(method)",
                          argument="method->args[{i}]",
                          pr=increase_printer(ipr),
                          tail=';', va_fmt="&method->args[{i}]",
                          valen_fmt="MT_NUM_ARGS(method) - {i}")
        pr('default:')
        ipr('return -EINVALIDMETHOD;')

    def put_event_processor(self, pr):
        pr(self.get_event_processor_prototype())
        pr('{')
        self.put_event_processor_body(increase_printer(pr))
        pr('}')

    def put_event_processor_body(self, pr):
        ipr = increase_printer(pr)
        pr("message_t msg = &event->msg;")
        pr("uint8_t *eargs = event->args;")
        pr("uint8_t *hargs = handler->args;")
        pr("uint8_t *vav = handler->ev_max_arg >= EV_NUM_ARGS(event) ? NULL :")
        ipr(    "&eargs[handler->ev_max_arg];")
        pr("uint16_t vac = handler->ev_max_arg >= EV_NUM_ARGS(event) ? 0 : ")
        ipr(    "EV_NUM_ARGS(event) - handler->ev_max_arg;")
        pr('')
        pr('switch(handler->called_method) {')
        predicate="{i} < handler->num_args && hargs[{i}] > 0 && hargs[{i}] < EV_NUM_ARGS(event)"
        argument="eargs[hargs[{i}] - 1]" # Count in handlers begins with 1
        for m in self._methods:
            pr('case {n}:'.format(n=m._num))
            ipr('return')
            m.call_format(predicate=predicate,
                          argument=argument,
                          pr=increase_printer(ipr),
                          tail=';',
                          valen="vac", va="vav" )
        pr('default:')
        ipr('return EOK;')

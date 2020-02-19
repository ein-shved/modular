from .printer import increase_printer

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


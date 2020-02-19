
class Argument (str):
    def __init__ (self, *args, **kwargs):
        self._default = 0
        str(*args, **kwargs)
        if 'default' in kwargs:
            self._default = kwargs['default']

    def default(self):
        return self._default


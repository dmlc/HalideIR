import io

class Formatter:
    def __init__(self) -> None:
        self._prefix = ''
        self._buf = io.StringIO()
        self._last = '\n'

    def prefix(self, prefix: str) -> '_Prefix':
        return _Prefix(self, prefix)
    
    def indent(self, amount: int) -> '_Prefix':
        return _Prefix(self, ' ' * amount)

    def write(self, value: str):
        if value == '':
            return

        if self._last == '\n':
            self._buf.write(self._prefix)

        self._buf.write(value[:-1].replace('\n', '\n' + self._prefix))
        self._buf.write(value[-1])
        self._last = value[-1]

    def writeln(self, value: str = ''):
        self.write(value + '\n')

    
    def __repr__(self):
        return self._buf.getvalue()

    def __str__(self):
        return self._buf.getvalue()

class _Prefix:
    def __init__(self, f: Formatter, prefix: str) -> None:
        self.f = f
        self.prefix = prefix

    def __enter__(self):
        self.f._prefix += self.prefix
    
    def __exit__(self, a, b, c):
        self.f._prefix = self.f._prefix[:-len(self.prefix)]


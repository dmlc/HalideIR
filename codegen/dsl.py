'''DSL to define a node heirarchy for an IR.'''
from typing import List, Dict, Any

class GenType:
    '''Any type that requires codegen.'''
    def __init__(self, name: str) -> None:
        self.name = name

class Primitive(GenType):
    '''A primitive.'''
    def __init__(self, name):
        self.name = name

int64 = Primitive('int64_t')
uint64 = Primitive('uint64_t')
double = float64 = Primitive('double')
string = Primitive('string')

class Attr:
    def __init__(self, name: str, type: GenType, doc: str = '') -> None:
        self.name = name
        self.type = type
        self.doc = doc

class Node(GenType):
    def __init__(self, name: str, type_key: str, doc: str = '') -> None:
        self.name = name
        self.type_key = type_key
        self.doc = doc
        self.attrs: List[Attr] = []

    def attr(self, name: str, type: GenType, doc: str = '') -> 'Node':
        if any(attr.name == name for attr in self.attrs):
            raise LookupError(f'Node type {self.name} already has an attribute named {name}')

        self.attrs.append(Attr(name, type, doc))

        return self

class API:
    def __init__(self, doc: str = '') -> None:
        self.doc = doc
        self.entries: List[Node] = []
    
    def __enter__(self) -> 'API':
        _set_api(self)
        return self

    def __exit__(self, a, b, c):
        global _default
        _set_api(_default)
    
_default = API()
_cur_api: API = _default
def _set_api(api: API):
    global _cur_api
    _cur_api = api
def _get_api() -> API:
    global _cur_api
    return _cur_api

def node(name: str, type_key: str, doc: str = '') -> Node:
    result = Node(name, type_key, doc)
    _get_api().entries.append(result)
    return result


'''C++ code generator for node heirarchy.'''

from .. import formatting, dsl, generators

def generate(api: dsl.API):
    result = formatting.Formatter()
    with result.prefix('/// '):
        result.writeln(api.doc)

    for entry in api.entries:

        result.writeln()

        if entry.doc:
            with result.prefix('/// '):
                result.writeln(entry.doc)

        result.writeln(f'class {entry.name}Node {{')
        result.writeln(f'public:')

        with result.indent(2):
            for attr in entry.attrs:

                if attr.doc:
                    with result.prefix('/// '):
                        result.writeln(attr.doc)

                result.writeln(f'{type_name(attr.type)} {attr.name};')
        
        result.writeln(f'}}')
        result.writeln(f'class {entry.name}: NodeRef<{entry.name}Node> {{}}')


    return result

def type_name(t: dsl.GenType) -> str:
    if t == dsl.int64:
        return 'int64_t'
    if t == dsl.uint64:
        return 'uint64_t'
    if t == dsl.string:
        return 'std::string'
    elif isinstance(t, dsl.Node):
        return t.name
    else:
        raise Exception(f'No c++ translation for: {t}')

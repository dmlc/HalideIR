from codegen.dsl import *
from codegen.generators import cpp

with API('Test API.') as api:
    Color = node('Color', '_color', 'A color.') \
        .attr('r', int64) \
        .attr('g', int64) \
        .attr('b', int64) \

    Apple = node('Apple', '_apple', 'A scrumptious apple.') \
        .attr('color', Color)

print(str(cpp.generate(api)))
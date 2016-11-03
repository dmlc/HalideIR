#include <iostream>
#include <sstream>

#include "IRPrinter.h"
#include "IROperator.h"

namespace Halide {

using std::ostream;
using std::vector;
using std::string;
using std::ostringstream;

ostream &operator<<(ostream &out, const Type &type) {
    switch (type.code()) {
    case Type::Int:
        out << "int";
        break;
    case Type::UInt:
        out << "uint";
        break;
    case Type::Float:
        out << "float";
        break;
    case Type::Handle:
        out << "handle";
        break;
    }
    out << type.bits();
    if (type.lanes() > 1) out << 'x' << type.lanes();
    return out;
}

ostream &operator<<(ostream &stream, const Expr &ir) {
    if (!ir.defined()) {
        stream << "(undefined)";
    } else {
        Internal::IRPrinter p(stream);
        p.print(ir);
    }
    return stream;
}

ostream &operator<<(ostream &out, const DeviceAPI &api) {
    switch (api) {
    case DeviceAPI::Host:
    case DeviceAPI::None:
        break;
    case DeviceAPI::Default_GPU:
        out << "<Default_GPU>";
        break;
    case DeviceAPI::CUDA:
        out << "<CUDA>";
        break;
    case DeviceAPI::OpenCL:
        out << "<OpenCL>";
        break;
    case DeviceAPI::OpenGLCompute:
        out << "<OpenGLCompute>";
        break;
    case DeviceAPI::GLSL:
        out << "<GLSL>";
        break;
    case DeviceAPI::Renderscript:
        out << "<Renderscript>";
        break;
    case DeviceAPI::Metal:
        out << "<Metal>";
        break;
    case DeviceAPI::Hexagon:
        out << "<Hexagon>";
        break;
    }
    return out;
}

namespace Internal {

ostream &operator<<(ostream &out, const ForType &type) {
    switch (type) {
    case ForType::Serial:
        out << "for";
        break;
    case ForType::Parallel:
        out << "parallel";
        break;
    case ForType::Unrolled:
        out << "unrolled";
        break;
    case ForType::Vectorized:
        out << "vectorized";
        break;
    }
    return out;
}

ostream &operator<<(ostream &stream, const Stmt &ir) {
    if (!ir.defined()) {
        stream << "(undefined)\n";
    } else {
        Internal::IRPrinter p(stream);
        p.print(ir);
    }
    return stream;
}


IRPrinter::IRPrinter(ostream &s) : stream(s), indent(0) {
    s.setf(std::ios::fixed, std::ios::floatfield);
}

void IRPrinter::print(Expr ir) {
    static const FType& f = vtable();
    f(ir, this);
}

void IRPrinter::print(Stmt ir) {
    static const FType& f = vtable();
    f(ir, this);
}


void IRPrinter::do_indent() {
    for (int i = 0; i < indent; i++) stream << ' ';
}

IRPrinter::FType& IRPrinter::vtable() {
  static FType inst;
  return inst;
}

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<IntImm>([](const IntImm *op, IRPrinter *p) {
    if (op->type == Int(32)) {
        p->stream << op->value;
    } else {
        p->stream << "(" << op->type << ")" << op->value;
    }
  });

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<UIntImm>([](const UIntImm *op, IRPrinter* p) {
    p->stream << "(" << op->type << ")" << op->value;
  });

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<FloatImm>([](const FloatImm *op, IRPrinter* p) {
  auto& stream = p->stream;
  switch (op->type.bits()) {
    case 64:
        stream << op->value;
        break;
    case 32:
        stream << op->value << 'f';
        break;
    case 16:
        stream << op->value << 'h';
        break;
    default:
        internal_error << "Bad bit-width for float: " << op->type << "\n";
    }
  });

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<StringImm>([](const StringImm *op, IRPrinter *p) {
    auto& stream = p->stream;
    stream << '"';
    for (size_t i = 0; i < op->value.size(); i++) {
        unsigned char c = op->value[i];
        if (c >= ' ' && c <= '~' && c != '\\' && c != '"') {
            stream << c;
        } else {
            stream << '\\';
            switch (c) {
            case '"':
                stream << '"';
                break;
            case '\\':
                stream << '\\';
                break;
            case '\t':
                stream << 't';
                break;
            case '\r':
                stream << 'r';
                break;
            case '\n':
                stream << 'n';
                break;
            default:
                string hex_digits = "0123456789ABCDEF";
                stream << 'x' << hex_digits[c >> 4] << hex_digits[c & 0xf];
            }
        }
    }
    stream << '"';
  });

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Cast>([](const Cast *op, IRPrinter *p) {
    p->stream << op->type << '(';
    p->print(op->value);
    p->stream << ')';
  })
.set_dispatch<Variable>([](const Variable *op, IRPrinter* p) {
    // omit the type
    // stream << op->name << "." << op->type;
    p->stream << op->name_hint;
  })
.set_dispatch<Add>([](const Add *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << " + ";
    p->print(op->b);
    p->stream << ')';
  })
.set_dispatch<Sub>([](const Sub *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << " - ";
    p->print(op->b);
    p->stream << ')';
  })
.set_dispatch<Mul>([](const Mul *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << "*";
    p->print(op->b);
    p->stream << ')';
  })
.set_dispatch<Div>([](const Div *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << "/";
    p->print(op->b);
    p->stream << ')';
  })
.set_dispatch<Mod>([](const Mod *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << " % ";
    p->print(op->b);
    p->stream << ')';
})
.set_dispatch<Min>([](const Min *op, IRPrinter* p) {
    p->stream << "min(";
    p->print(op->a);
    p->stream << ", ";
    p->print(op->b);
    p->stream << ")";
})
.set_dispatch<Max>([](const Max *op, IRPrinter* p) {
    p->stream << "max(";
    p->print(op->a);
    p->stream << ", ";
    p->print(op->b);
    p->stream << ")";
})
.set_dispatch<EQ>([](const EQ *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << " == ";
    p->print(op->b);
    p->stream << ')';
})
.set_dispatch<NE>([](const NE *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << " != ";
    p->print(op->b);
    p->stream << ')';
})
.set_dispatch<LT>([](const LT *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << " < ";
    p->print(op->b);
    p->stream << ')';
})
.set_dispatch<LE>([](const LE *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << " <= ";
    p->print(op->b);
    p->stream << ')';
})
.set_dispatch<GT>([](const GT *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << " > ";
    p->print(op->b);
    p->stream << ')';
})
.set_dispatch<GE>([](const GE *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << " >= ";
    p->print(op->b);
    p->stream << ')';
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<And>([](const And *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << " && ";
    p->print(op->b);
    p->stream << ')';
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Or>([](const Or *op, IRPrinter* p) {
    p->stream << '(';
    p->print(op->a);
    p->stream << " || ";
    p->print(op->b);
    p->stream << ')';
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Not>([](const Not *op, IRPrinter* p) {
    p->stream << '!';
    p->print(op->a);
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Select>([](const Select *op, IRPrinter* p) {
    p->stream << "select(";
    p->print(op->condition);
    p->stream << ", ";
    p->print(op->true_value);
    p->stream << ", ";
    p->print(op->false_value);
    p->stream << ")";
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Load>([](const Load *op, IRPrinter* p) {
    p->stream << op->buffer_var << "[";
    p->print(op->index);
    p->stream << "]";
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Ramp>([](const Ramp *op, IRPrinter* p) {
    p->stream << "ramp(";
    p->print(op->base);
    p->stream << ", ";
    p->print(op->stride);
    p->stream << ", " << op->lanes << ")";
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Broadcast>([](const Broadcast *op, IRPrinter* p) {
    p->stream << "x" << op->lanes << "(";
    p->print(op->value);
    p->stream << ")";
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Call>([](const Call *op, IRPrinter* p) {
    // Special-case some intrinsics for readability
    if (op->is_intrinsic(Call::extract_buffer_host)) {
        p->print(op->args[0]);
        p->stream << ".host";
        return;
    } else if (op->is_intrinsic(Call::extract_buffer_min)) {
        p->print(op->args[0]);
        p->stream << ".min[" << op->args[1] << "]";
        return;
    } else if (op->is_intrinsic(Call::extract_buffer_max)) {
        p->print(op->args[0]);
        p->stream << ".max[" << op->args[1] << "]";
        return;
    }

    // TODO: Print indication of C vs C++?
    p->stream << op->name << "(";
    for (size_t i = 0; i < op->args.size(); i++) {
        p->print(op->args[i]);
        if (i < op->args.size() - 1) {
            p->stream << ", ";
        }
    }
    p->stream << ")";
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Let>([](const Let *op, IRPrinter* p) {
    p->stream << "(let " << op->var << " = ";
    p->print(op->value);
    p->stream << " in ";
    p->print(op->body);
    p->stream << ")";
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<LetStmt>([](const LetStmt *op, IRPrinter* p) {
    p->do_indent();
    p->stream << "let " << op->var << " = ";
    p->print(op->value);
    p->stream << '\n';

    p->print(op->body);
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<AssertStmt>([](const AssertStmt *op, IRPrinter* p) {
    p->do_indent();
    p->stream << "assert(";
    p->print(op->condition);
    p->stream << ", ";
    p->print(op->message);
    p->stream << ")\n";
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<ProducerConsumer>([](const ProducerConsumer *op, IRPrinter* p) {
    if (op->is_producer) {
        p->do_indent();
        p->stream << "produce " << op->func->name() << " {\n";
        p->indent += 2;
        p->print(op->body);
        p->indent -= 2;
        p->do_indent();
        p->stream << "}\n";
    } else {
        p->print(op->body);
    }

});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<For>([](const For *op, IRPrinter* p) {

    p->do_indent();
    p->stream << op->for_type << op->device_api << " (" << op->loop_var << ", ";
    p->print(op->min);
    p->stream << ", ";
    p->print(op->extent);
    p->stream << ") {\n";

    p->indent += 2;
    p->print(op->body);
    p->indent -= 2;

    p->do_indent();
    p->stream << "}\n";
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Store>([](const Store *op, IRPrinter* p) {
    p->do_indent();
    p->stream << op->buffer_var << "[";
    p->print(op->index);
    p->stream << "] = ";
    p->print(op->value);
    p->stream << '\n';
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Provide>([](const Provide *op, IRPrinter* p) {
    p->do_indent();
    p->stream << op->func->name() << "(";
    for (size_t i = 0; i < op->args.size(); i++) {
        p->print(op->args[i]);
        if (i < op->args.size() - 1) p->stream << ", ";
    }
    p->stream << ") = ";
    if (op->values.size() > 1) {
        p->stream << "{";
    }
    for (size_t i = 0; i < op->values.size(); i++) {
        if (i > 0) {
            p->stream << ", ";
        }
        p->print(op->values[i]);
    }
    if (op->values.size() > 1) {
        p->stream << "}";
    }

    p->stream << '\n';
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Allocate>([](const Allocate *op, IRPrinter* p) {
    p->do_indent();
    p->stream << "allocate " << op->buffer_var << "[" << op->type;
    for (size_t i = 0; i < op->extents.size(); i++) {
        p->stream << " * ";
        p->print(op->extents[i]);
    }
    p->stream << "]";
    if (!is_one(op->condition)) {
        p->stream << " if ";
        p->print(op->condition);
    }
    if (op->new_expr.defined()) {
        p->stream << "\n custom_new { " << op->new_expr << " }";
    }
    if (!op->free_function.empty()) {
        p->stream << "\n custom_delete { " << op->free_function << "(<args>); }";
    }
    p->stream << "\n";
    p->print(op->body);
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Free>([](const Free *op, IRPrinter* p) {
    p->do_indent();
    p->stream << "free " << op->buffer_var;
    p->stream << '\n';
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Realize>([](const Realize *op, IRPrinter* p) {
    p->do_indent();
    p->stream << "realize " << op->func->name() << "(";
    for (size_t i = 0; i < op->bounds.size(); i++) {
        p->stream << "[";
        p->print(op->bounds[i]->min);
        p->stream << ", ";
        p->print(op->bounds[i]->extent);
        p->stream << "]";
        if (i < op->bounds.size() - 1) p->stream << ", ";
    }
    p->stream << ")";
    if (!is_one(op->condition)) {
        p->stream << " if ";
        p->print(op->condition);
    }
    p->stream << " {\n";

    p->indent += 2;
    p->print(op->body);
    p->indent -= 2;

    p->do_indent();
    p->stream << "}\n";
});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Block>([](const Block *op, IRPrinter* p) {
    p->print(op->first);
    if (op->rest.defined()) p->print(op->rest);
  });

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<IfThenElse>([](const IfThenElse *op, IRPrinter* p) {
    p->do_indent();
    while (1) {
        p->stream << "if (" << op->condition << ") {\n";
        p->indent += 2;
        p->print(op->then_case);
        p->indent -= 2;

        if (!op->else_case.defined()) {
            break;
        }

        if (const IfThenElse *nested_if = op->else_case.as<IfThenElse>()) {
            p->do_indent();
            p->stream << "} else ";
            op = nested_if;
        } else {
            p->do_indent();
            p->stream << "} else {\n";
            p->indent += 2;
            p->print(op->else_case);
            p->indent -= 2;
            break;
        }
    }

    p->do_indent();
    p->stream << "}\n";

});

TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
.set_dispatch<Evaluate>([](const Evaluate *op, IRPrinter* p) {
    p->do_indent();
    p->print(op->value);
    p->stream << "\n";
  });

}}

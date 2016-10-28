#include "IRMutator.h"

namespace Halide {
namespace Internal {

using std::vector;

Expr IRMutator::mutate(Expr e) {
    if (e.defined()) {
        e.accept(this);
    } else {
        expr_ = Expr();
    }
    stmt_ = Stmt();
    if (no_mutation_) {
        return e;
    } else {
        return expr_;
    }
}

Stmt IRMutator::mutate(Stmt s) {
    if (s.defined()) {
        s.accept(this);
    } else {
        stmt_ = Stmt();
    }
    expr_ = Expr();
    if (no_mutation_) {
        return s;
    } else {
       return stmt_;
    }
}

void IRMutator::visit(const IntImm *op)   {return_self();}
void IRMutator::visit(const UIntImm *op)   {return_self();}
void IRMutator::visit(const FloatImm *op) {return_self();}
void IRMutator::visit(const StringImm *op) {return_self();}
void IRMutator::visit(const Variable *op) {return_self();}

void IRMutator::visit(const Cast *op) {
    Expr value = mutate(op->value);
    if (value.same_as(op->value)) {
        return_self();
    } else {
        return_value(Cast::make(op->type, value));
    }
}

// use macro to access private function.
#define MUTATE_BINARY_OP(op, T)                   \
  Expr a = mutate(op->a);                         \
  Expr b = mutate(op->b);                         \
  if (a.same_as(op->a) &&                         \
      b.same_as(op->b)) {                         \
    return_self();                                \
  } else {                                        \
    return_value(T::make(a, b));                  \
  }                                               \

void IRMutator::visit(const Add *op) {
  MUTATE_BINARY_OP(op, Add);
}
void IRMutator::visit(const Sub *op) {
  MUTATE_BINARY_OP(op, Sub);
}
void IRMutator::visit(const Mul *op)  {
  MUTATE_BINARY_OP(op, Mul);
}
void IRMutator::visit(const Div *op)  {
  MUTATE_BINARY_OP(op, Div);
}
void IRMutator::visit(const Mod *op) {
  MUTATE_BINARY_OP(op, Mod);
}
void IRMutator::visit(const Min *op) {
  MUTATE_BINARY_OP(op, Min);
}
void IRMutator::visit(const Max *op) {
  MUTATE_BINARY_OP(op, Max);
}
void IRMutator::visit(const EQ *op) {
  MUTATE_BINARY_OP(op, EQ);
}
void IRMutator::visit(const NE *op) {
  MUTATE_BINARY_OP(op, NE);
}
void IRMutator::visit(const LT *op) {
  MUTATE_BINARY_OP(op, LT);
}
void IRMutator::visit(const LE *op) {
  MUTATE_BINARY_OP(op, LE);
}
void IRMutator::visit(const GT *op) {
  MUTATE_BINARY_OP(op, GT);
}
void IRMutator::visit(const GE *op) {
  MUTATE_BINARY_OP(op, GE);
}
void IRMutator::visit(const And *op) {
  MUTATE_BINARY_OP(op, And);
}
void IRMutator::visit(const Or *op) {
  MUTATE_BINARY_OP(op, Or);
}

void IRMutator::visit(const Not *op) {
    Expr a = mutate(op->a);
    if (a.same_as(op->a)) return_self();
    else return_value(Not::make(a));
}

void IRMutator::visit(const Select *op)  {
    Expr cond = mutate(op->condition);
    Expr t = mutate(op->true_value);
    Expr f = mutate(op->false_value);
    if (cond.same_as(op->condition) &&
        t.same_as(op->true_value) &&
        f.same_as(op->false_value)) {
        return_self();
    } else {
        return_value(Select::make(cond, t, f));
    }
}

void IRMutator::visit(const Load *op) {
    Expr index = mutate(op->index);
    if (index.same_as(op->index)) {
        return_self();
    } else {
      return_value(Load::make(op->type, op->buffer_var, index));
    }
}

void IRMutator::visit(const Ramp *op) {
    Expr base = mutate(op->base);
    Expr stride = mutate(op->stride);
    if (base.same_as(op->base) &&
        stride.same_as(op->stride)) {
        return_self();
    } else {
        return_value(Ramp::make(base, stride, op->lanes));
    }
}

void IRMutator::visit(const Broadcast *op) {
    Expr value = mutate(op->value);
    if (value.same_as(op->value)) return_self();
    else return_value(Broadcast::make(value, op->lanes));
}

void IRMutator::visit(const Call *op) {
    vector<Expr > new_args(op->args.size());
    bool changed = false;

    // Mutate the args
    for (size_t i = 0; i < op->args.size(); i++) {
        Expr old_arg = op->args[i];
        Expr new_arg = mutate(old_arg);
        if (!new_arg.same_as(old_arg)) changed = true;
        new_args[i] = new_arg;
    }

    if (!changed) {
        return_self();
    } else {
        return_value(Call::make(op->type, op->name, new_args, op->call_type,
                                op->func, op->value_index));
    }
}

void IRMutator::visit(const Let *op) {
    Expr value = mutate(op->value);
    Expr body = mutate(op->body);
    if (value.same_as(op->value) &&
        body.same_as(op->body)) {
        return_self();
    } else {
        return_value(Let::make(op->var, value, body));
    }
}

void IRMutator::visit(const LetStmt *op) {
    Expr value = mutate(op->value);
    Stmt body = mutate(op->body);
    if (value.same_as(op->value) &&
        body.same_as(op->body)) {
        return_self();
    } else {
      return_value(LetStmt::make(op->var, value, body));
    }
}

void IRMutator::visit(const AssertStmt *op) {
    Expr condition = mutate(op->condition);
    Expr message = mutate(op->message);

    if (condition.same_as(op->condition) && message.same_as(op->message)) {
        return_self();
    } else {
        return_value(AssertStmt::make(condition, message));
    }
}

void IRMutator::visit(const ProducerConsumer *op) {
    Stmt body = mutate(op->body);
    if (body.same_as(op->body)) {
        return_self();
    } else {
        return_value(ProducerConsumer::make(op->func, op->is_producer, body));
    }
}

void IRMutator::visit(const For *op) {
    Expr min = mutate(op->min);
    Expr extent = mutate(op->extent);
    Stmt body = mutate(op->body);
    if (min.same_as(op->min) &&
        extent.same_as(op->extent) &&
        body.same_as(op->body)) {
        return_self();
    } else {
        return_value(For::make(
            op->loop_var, min, extent, op->for_type, op->device_api, body));
    }
}

void IRMutator::visit(const Store *op) {
    Expr value = mutate(op->value);
    Expr index = mutate(op->index);
    if (value.same_as(op->value) && index.same_as(op->index)) {
        return_self();
    } else {
        return_value(Store::make(op->buffer_var, value, index));
    }
}

void IRMutator::visit(const Provide *op) {
    vector<Expr> new_args(op->args.size());
    vector<Expr> new_values(op->values.size());
    bool changed = false;

    // Mutate the args
    for (size_t i = 0; i < op->args.size(); i++) {
        Expr old_arg = op->args[i];
        Expr new_arg = mutate(old_arg);
        if (!new_arg.same_as(old_arg)) changed = true;
        new_args[i] = new_arg;
    }

    for (size_t i = 0; i < op->values.size(); i++) {
        Expr old_value = op->values[i];
        Expr new_value = mutate(old_value);
        if (!new_value.same_as(old_value)) changed = true;
        new_values[i] = new_value;
    }

    if (!changed) {
        return_self();
    } else {
        return_value(Provide::make(op->name, new_values, new_args));
    }
}

void IRMutator::visit(const Allocate *op) {
    std::vector<Expr> new_extents;
    bool all_extents_unmodified = true;
    for (size_t i = 0; i < op->extents.size(); i++) {
        new_extents.push_back(mutate(op->extents[i]));
        all_extents_unmodified &= new_extents[i].same_as(op->extents[i]);
    }
    Stmt body = mutate(op->body);
    Expr condition = mutate(op->condition);
    Expr new_expr;
    if (op->new_expr.defined()) {
        new_expr = mutate(op->new_expr);
    }
    if (all_extents_unmodified &&
        body.same_as(op->body) &&
        condition.same_as(op->condition) &&
        new_expr.same_as(op->new_expr)) {
        return_self();
    } else {
        return_value(Allocate::make(op->buffer_var, op->type, new_extents, condition, body, new_expr, op->free_function));
    }
}

void IRMutator::visit(const Free *op) {
    return_self();
}

void IRMutator::visit(const Realize *op) {
    Region new_bounds;
    bool bounds_changed = false;

    // Mutate the bounds
    for (size_t i = 0; i < op->bounds.size(); i++) {
        Expr old_min    = op->bounds[i]->min;
        Expr old_extent = op->bounds[i]->extent;
        Expr new_min    = mutate(old_min);
        Expr new_extent = mutate(old_extent);
        if (!new_min.same_as(old_min))       bounds_changed = true;
        if (!new_extent.same_as(old_extent)) bounds_changed = true;
        new_bounds.push_back(Range(new_min, new_extent));
    }

    Stmt body = mutate(op->body);
    Expr condition = mutate(op->condition);
    if (!bounds_changed &&
        body.same_as(op->body) &&
        condition.same_as(op->condition)) {
        return_self();
    } else {
        return_value(Realize::make(op->func, op->types, new_bounds,
                                   condition, body));
    }
}

void IRMutator::visit(const Block *op) {
    Stmt first = mutate(op->first);
    Stmt rest = mutate(op->rest);
    if (first.same_as(op->first) &&
        rest.same_as(op->rest)) {
        return_self();
    } else {
        return_value(Block::make(first, rest));
    }
}

void IRMutator::visit(const IfThenElse *op) {
    Expr condition = mutate(op->condition);
    Stmt then_case = mutate(op->then_case);
    Stmt else_case = mutate(op->else_case);
    if (condition.same_as(op->condition) &&
        then_case.same_as(op->then_case) &&
        else_case.same_as(op->else_case)) {
        return_self();
    } else {
        return_value(IfThenElse::make(condition, then_case, else_case));
    }
}

void IRMutator::visit(const Evaluate *op) {
    Expr v = mutate(op->value);
    if (v.same_as(op->value)) {
        return_self();
    } else {
        return_value(Evaluate::make(v));
    }
}


Stmt IRGraphMutator::mutate(Stmt s) {
    auto iter = stmt_replacements.find(s);
    if (iter != stmt_replacements.end()) {
        return iter->second;
    }
    Stmt new_s = IRMutator::mutate(s);
    stmt_replacements[s] = new_s;
    return new_s;
}

Expr IRGraphMutator::mutate(Expr e) {
    auto iter = expr_replacements.find(e);
    if (iter != expr_replacements.end()) {
        return iter->second;
    }
    Expr new_e = IRMutator::mutate(e);
    expr_replacements[e] = new_e;
    return new_e;
}

}
}

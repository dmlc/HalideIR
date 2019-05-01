#include "IR.h"
//#include "IRPrinter.h"
#include "IRVisitor.h"

namespace HalideIR {

EXPORT Expr::Expr(int8_t x) : Expr(Internal::IntImm::make(Int(8), x)) {}
EXPORT Expr::Expr(int16_t x) : Expr(Internal::IntImm::make(Int(16), x)) {}
EXPORT Expr::Expr(int32_t x) : Expr(Internal::IntImm::make(Int(32), x)) {}
EXPORT Expr::Expr(int64_t x) : Expr(Internal::IntImm::make(Int(64), x)) {}
EXPORT Expr::Expr(uint8_t x) : Expr(Internal::UIntImm::make(UInt(8), x)) {}
EXPORT Expr::Expr(uint16_t x) : IRHandle(Internal::UIntImm::make(UInt(16), x)) {}
EXPORT Expr::Expr(uint32_t x) : IRHandle(Internal::UIntImm::make(UInt(32), x)) {}
EXPORT Expr::Expr(uint64_t x) : IRHandle(Internal::UIntImm::make(UInt(64), x)) {}
EXPORT Expr::Expr(float16_t x) : IRHandle(Internal::FloatImm::make(Float(16), (double)x)) {}
EXPORT Expr::Expr(float x) : IRHandle(Internal::FloatImm::make(Float(32), x)) {}
EXPORT Expr::Expr(double x) : IRHandle(Internal::FloatImm::make(Float(64), x)) {}
EXPORT Expr::Expr(const std::string &s) : IRHandle(Internal::StringImm::make(s)) {}

EXPORT VarExpr::VarExpr(const std::string &name_hint,  Type t)
    : VarExpr(Internal::Variable::make(t, name_hint)) {}

namespace Internal {

#if defined(__clang__)
__attribute__((no_sanitize("undefined")))
#endif
EXPORT Expr IntImm::make(Type t, int64_t value) {
    internal_assert(t.is_int() && t.is_scalar())
        << "IntImm must be a scalar Int\n";
    internal_assert(t.bits() == 8 || t.bits() == 16 || t.bits() == 32 || t.bits() == 64)
        << "IntImm must be 8, 16, 32, or 64-bit\n";

    // Normalize the value by dropping the high bits
    value <<= (64 - t.bits());
    // Then sign-extending to get them back
    value >>= (64 - t.bits());

    NodePtr<IntImm> node = make_node<IntImm>();
    node->type = t;
    node->value = value;
    return Expr(node);
}

EXPORT Expr UIntImm::make(Type t, uint64_t value) {
    internal_assert(t.is_uint() && t.is_scalar())
        << "UIntImm must be a scalar UInt\n";
    internal_assert(t.bits() == 1 || t.bits() == 8 || t.bits() == 16 || t.bits() == 32 || t.bits() == 64)
        << "UIntImm must be 1, 8, 16, 32, or 64-bit\n";

    // Normalize the value by dropping the high bits
    value <<= (64 - t.bits());
    value >>= (64 - t.bits());

    NodePtr<UIntImm> node = make_node<UIntImm>();
    node->type = t;
    node->value = value;
    return Expr(node);
}

EXPORT Expr FloatImm::make(Type t, double value) {
  internal_assert(t.is_scalar())
      << "FloatImm must be a scalar\n";
  NodePtr<FloatImm> node = make_node<FloatImm>();
  node->type = t;
  switch (t.bits()) {
    case 16:
      node->value = (double)((float16_t)value);
      break;
    case 32:
      node->value = (float)value;
      break;
    case 64:
      node->value = value;
      break;
    default:
      internal_error << "FloatImm must be 16, 32, or 64-bit\n";
  }

  return Expr(node);
}

EXPORT Expr StringImm::make(const std::string &val) {
    NodePtr<StringImm> node = make_node<StringImm>();
    node->type = type_of<const char *>();
    node->value = val;
    return Expr(node);
}


EXPORT Expr Cast::make(Type t, Expr v) {
    internal_assert(v.defined()) << "Cast of undefined\n";
    internal_assert(t.lanes() == v.type().lanes()) << "Cast may not change vector widths\n";

    NodePtr<Cast> node = make_node<Cast>();
    node->type = t;
    node->value = std::move(v);
    return Expr(node);
}


EXPORT Expr And::make(Expr a, Expr b) {
    internal_assert(a.defined()) << "And of undefined\n";
    internal_assert(b.defined()) << "And of undefined\n";
    internal_assert(a.type().is_bool()) << "lhs of And is not a bool\n";
    internal_assert(b.type().is_bool()) << "rhs of And is not a bool\n";
    internal_assert(a.type() == b.type()) << "And of mismatched types\n";

    NodePtr<And> node = make_node<And>();
    node->type = Bool(a.type().lanes());
    node->a = std::move(a);
    node->b = std::move(b);
    return Expr(node);
}

EXPORT Expr Or::make(Expr a, Expr b) {
    internal_assert(a.defined()) << "Or of undefined\n";
    internal_assert(b.defined()) << "Or of undefined\n";
    internal_assert(a.type().is_bool()) << "lhs of Or is not a bool\n";
    internal_assert(b.type().is_bool()) << "rhs of Or is not a bool\n";
    internal_assert(a.type() == b.type()) << "Or of mismatched types\n";

    NodePtr<Or> node = make_node<Or>();
    node->type = Bool(a.type().lanes());
    node->a = std::move(a);
    node->b = std::move(b);
    return Expr(node);
}

EXPORT Expr Not::make(Expr a) {
    internal_assert(a.defined()) << "Not of undefined\n";
    internal_assert(a.type().is_bool()) << "argument of Not is not a bool\n";
    NodePtr<Not> node = make_node<Not>();
    node->type = Bool(a.type().lanes());
    node->a = std::move(a);
    return Expr(node);
}

EXPORT Expr Select::make(Expr condition, Expr true_value, Expr false_value) {
    internal_assert(condition.defined()) << "Select of undefined\n";
    internal_assert(true_value.defined()) << "Select of undefined\n";
    internal_assert(false_value.defined()) << "Select of undefined\n";
    internal_assert(condition.type().is_bool()) << "First argument to Select is not a bool: " << condition.type() << "\n";
    internal_assert(false_value.type() == true_value.type()) << "Select of mismatched types\n";
    internal_assert(condition.type().is_scalar() ||
                    condition.type().lanes() == true_value.type().lanes())
        << "In Select, vector lanes of condition must either be 1, or equal to vector lanes of arguments\n";

    NodePtr<Select> node = make_node<Select>();
    node->type = true_value.type();
    node->condition = std::move(condition);
    node->true_value = std::move(true_value);
    node->false_value = std::move(false_value);
    return Expr(node);
}


EXPORT Expr Load::make(Type type, VarExpr buffer_var, Expr index, Expr predicate) {
    internal_assert(predicate.defined()) << "Load with undefined predicate\n";
    internal_assert(index.defined()) << "Load of undefined\n";
    internal_assert(type.lanes() == index.type().lanes()) << "Vector lanes of Load must match vector lanes of index\n";
    internal_assert(type.lanes() == predicate.type().lanes())
        << "Vector lanes of Load must match vector lanes of predicate\n";

    NodePtr<Load> node = make_node<Load>();
    node->type = type;
    node->buffer_var = std::move(buffer_var);
    node->index = std::move(index);
    node->predicate = std::move(predicate);

    return Expr(node);
}


EXPORT Expr Ramp::make(Expr base, Expr stride, int lanes) {
    internal_assert(base.defined()) << "Ramp of undefined\n";
    internal_assert(stride.defined()) << "Ramp of undefined\n";
    internal_assert(base.type().is_scalar()) << "Ramp with vector base\n";
    internal_assert(stride.type().is_scalar()) << "Ramp with vector stride\n";
    internal_assert(lanes > 1) << "Ramp of lanes <= 1\n";
    internal_assert(stride.type() == base.type()) << "Ramp of mismatched types\n";
    NodePtr<Ramp> node = make_node<Ramp>();
    internal_assert(base.defined()) << "Ramp of undefined\n";
    node->type = base.type().with_lanes(lanes);
    node->base = base;
    node->stride = stride;
    node->lanes = lanes;
    return Expr(node);
}

EXPORT Expr Broadcast::make(Expr value, int lanes) {
    internal_assert(value.defined()) << "Broadcast of undefined\n";
    internal_assert(value.type().is_scalar()) << "Broadcast of vector\n";
    internal_assert(lanes != 1) << "Broadcast of lanes 1\n";
    NodePtr<Broadcast> node = make_node<Broadcast>();
    node->type = value.type().with_lanes(lanes);
    node->value = std::move(value);
    node->lanes = lanes;
    return Expr(node);
}

EXPORT Expr Let::make(VarExpr var, Expr value, Expr body) {
    internal_assert(value.defined()) << "Let of undefined\n";
    internal_assert(body.defined()) << "Let of undefined\n";
    internal_assert(value.type() == var.type()) << "Let var mismatch\n";
    NodePtr<Let> node = make_node<Let>();
    node->type = body.type();
    node->var = std::move(var);
    node->value = std::move(value);
    node->body = std::move(body);
    return Expr(node);
}

EXPORT Stmt LetStmt::make(VarExpr var, Expr value, Stmt body) {
    internal_assert(value.defined()) << "Let of undefined\n";
    internal_assert(body.defined()) << "Let of undefined\n";
    internal_assert(value.type() == var.type()) << "Let var mismatch\n";
    NodePtr<LetStmt> node = make_node<LetStmt>();
    node->var = std::move(var);
    node->value = std::move(value);
    node->body = std::move(body);
    return Stmt(node);
}

EXPORT Stmt AttrStmt::make(NodeRef node, std::string attr_key, Expr value, Stmt body) {
  auto n = make_node<AttrStmt>();
  n->node = node;
  n->attr_key = std::move(attr_key);
  n->value = std::move(value);
  n->body = std::move(body);
  return Stmt(n);
}

EXPORT Stmt AssertStmt::make(Expr condition, Expr message, Stmt body) {
    internal_assert(condition.defined()) << "AssertStmt of undefined\n";
    internal_assert(message.type() == Int(32) ||
                    message.as<StringImm>()) << "AssertStmt message must be an int or string:"
                                           << message << "\n";

    NodePtr<AssertStmt> node = make_node<AssertStmt>();
    node->condition = std::move(condition);
    node->message = std::move(message);
    node->body = std::move(body);
    return Stmt(node);
}

EXPORT Stmt ProducerConsumer::make(FunctionRef func, bool is_producer, Stmt body) {
    internal_assert(body.defined()) << "ProducerConsumer of undefined\n";

    NodePtr<ProducerConsumer> node = make_node<ProducerConsumer>();
    node->func = std::move(func);
    node->is_producer = is_producer;
    node->body = std::move(body);
    return Stmt(node);
}

EXPORT Stmt For::make(VarExpr loop_var,
               Expr min, Expr extent,
               ForType for_type, DeviceAPI device_api,
               Stmt body) {
    internal_assert(min.defined()) << "For of undefined\n";
    internal_assert(extent.defined()) << "For of undefined\n";
    internal_assert(min.type().is_scalar()) << "For with vector min\n";
    internal_assert(extent.type().is_scalar()) << "For with vector extent\n";
    internal_assert(loop_var.type().is_scalar()) << "For with vector loop_var";
    internal_assert(body.defined()) << "For of undefined\n";

    NodePtr<For> node = make_node<For>();
    node->loop_var = std::move(loop_var);
    node->min = std::move(min);
    node->extent = std::move(extent);
    node->for_type = for_type;
    node->device_api = device_api;
    node->body = std::move(body);
    return Stmt(node);
}

EXPORT Stmt Store::make(VarExpr buffer_var, Expr value, Expr index, Expr predicate) {
    internal_assert(value.defined()) << "Store of undefined\n";
    internal_assert(index.defined()) << "Store of undefined\n";
    internal_assert(predicate.defined()) << "Store with undefined predicate\n";
    internal_assert(value.type().lanes() == index.type().lanes()) << "Vector lanes of Store must match vector lanes of index\n";
    internal_assert(value.type().lanes() == predicate.type().lanes())
        << "Vector lanes of Store must match vector lanes of predicate\n";

    NodePtr<Store> node = make_node<Store>();
    node->buffer_var = std::move(buffer_var);
    node->value = std::move(value);
    node->index = std::move(index);
    node->predicate = std::move(predicate);
    return Stmt(node);
}

EXPORT Stmt Provide::make(FunctionRef func, int value_index, Expr value, Array<Expr> args) {
    internal_assert(value_index >=0 && value_index < func->num_outputs())
        << "value index output function return value bound";
    internal_assert(value.defined()) << "Provide of undefined value\n";
    for (size_t i = 0; i < args.size(); i++) {
        internal_assert(args[i].defined()) << "Provide to undefined location\n";
    }

    NodePtr<Provide> node = make_node<Provide>();
    node->func = std::move(func);
    node->value_index = value_index;
    node->value = std::move(value);
    node->args = std::move(args);
    return Stmt(node);
}

EXPORT Stmt Allocate::make(VarExpr buffer_var,
                    Type type,
                    Array<Expr> extents,
                    Expr condition, Stmt body,
                    Expr new_expr, std::string free_function) {
    for (size_t i = 0; i < extents.size(); i++) {
        internal_assert(extents[i].defined()) << "Allocate of undefined extent\n";
        internal_assert(extents[i].type().is_scalar() == 1) << "Allocate of vector extent\n";
    }
    internal_assert(body.defined()) << "Allocate of undefined\n";
    internal_assert(condition.defined()) << "Allocate with undefined condition\n";
    internal_assert(condition.type().is_bool()) << "Allocate condition is not boolean\n";

    NodePtr<Allocate> node = make_node<Allocate>();
    node->buffer_var = std::move(buffer_var);
    node->type = type;
    node->extents = std::move(extents);
    node->new_expr = std::move(new_expr);
    node->free_function = free_function;
    node->condition = std::move(condition);
    node->body = std::move(body);
    return Stmt(node);
}

int32_t Allocate::constant_allocation_size(const Array<Expr> &extents, const std::string& name) {
    int64_t result = 1;

    for (size_t i = 0; i < extents.size(); i++) {
        if (const IntImm *int_size = extents[i].as<IntImm>()) {
            // Check if the individual dimension is > 2^31 - 1. Not
            // currently necessary because it's an int32_t, which is
            // always smaller than 2^31 - 1. If we ever upgrade the
            // type of IntImm but not the maximum allocation size, we
            // should re-enable this.
            /*
            if ((int64_t)int_size->value > (((int64_t)(1)<<31) - 1)) {
                user_error
                    << "Dimension " << i << " for allocation " << name << " has size " <<
                    int_size->value << " which is greater than 2^31 - 1.";
            }
            */
            result *= int_size->value;
            if (result > (static_cast<int64_t>(1)<<31) - 1) {
                user_error
                    << "Total size for allocation " << name
                    << " is constant but exceeds 2^31 - 1.\n";
            }
        } else {
            return 0;
        }
    }

    return static_cast<int32_t>(result);
}

int32_t Allocate::constant_allocation_size() const {
    return Allocate::constant_allocation_size(
        extents, buffer_var->name_hint);
}

Stmt Free::make(VarExpr buffer_var) {
    NodePtr<Free> node = make_node<Free>();
    node->buffer_var = buffer_var;
    return Stmt(node);
}

Stmt Realize::make(FunctionRef func, int value_index, Type type,
                   Region bounds, Expr condition, Stmt body) {
    for (size_t i = 0; i < bounds.size(); i++) {
        internal_assert(bounds[i]->min.defined()) << "Realize of undefined\n";
        internal_assert(bounds[i]->extent.defined()) << "Realize of undefined\n";
        internal_assert(bounds[i]->min.type().is_scalar()) << "Realize of vector size\n";
        internal_assert(bounds[i]->extent.type().is_scalar()) << "Realize of vector size\n";
    }
    internal_assert(body.defined()) << "Realize of undefined\n";
    internal_assert(condition.defined()) << "Realize with undefined condition\n";
    internal_assert(condition.type().is_bool()) << "Realize condition is not boolean\n";

    NodePtr<Realize> node = make_node<Realize>();
    node->func = std::move(func);
    node->value_index = value_index;
    node->type = type;
    node->bounds = std::move(bounds);
    node->condition = std::move(condition);
    node->body = std::move(body);
    return Stmt(node);
}

Stmt Prefetch::make(FunctionRef func, int value_index, Type type, Region bounds) {
    for (size_t i = 0; i < bounds.size(); i++) {
        internal_assert(bounds[i]->min.defined()) << "Prefetch of undefined\n";
        internal_assert(bounds[i]->extent.defined()) << "Prefetch of undefined\n";
        internal_assert(bounds[i]->min.type().is_scalar()) << "Prefetch of vector size\n";
        internal_assert(bounds[i]->extent.type().is_scalar()) << "Prefetch of vector size\n";
    }
    NodePtr<Prefetch> node = make_node<Prefetch>();
    node->func = std::move(func);
    node->value_index = value_index;
    node->type = type;
    node->bounds = std::move(bounds);
    return Stmt(node);
}

Stmt Block::make(Stmt first, Stmt rest) {
    internal_assert(first.defined()) << "Block of undefined\n";
    internal_assert(rest.defined()) << "Block of undefined\n";

    NodePtr<Block> node = make_node<Block>();

    if (const Block *b = first.as<Block>()) {
        // Use a canonical block nesting order
        node->first = b->first;
        node->rest  = Block::make(b->rest, rest);
    } else {
        node->first = first;
        node->rest = rest;
    }

    return Stmt(node);
}

Stmt Block::make(const std::vector<Stmt> &stmts) {
    if (stmts.empty()) {
        return Stmt();
    }
    Stmt result = stmts.back();
    for (size_t i = stmts.size()-1; i > 0; i--) {
        result = Block::make(stmts[i-1], result);
    }
    return result;
}

Stmt IfThenElse::make(Expr condition, Stmt then_case, Stmt else_case) {
    internal_assert(condition.defined() && then_case.defined()) << "IfThenElse of undefined\n";
    // else_case may be null.

    NodePtr<IfThenElse> node = make_node<IfThenElse>();
    node->condition = std::move(condition);
    node->then_case = std::move(then_case);
    node->else_case = std::move(else_case);
    return Stmt(node);
}

EXPORT Stmt Evaluate::make(Expr v) {
    internal_assert(v.defined()) << "Evaluate of undefined\n";

    NodePtr<Evaluate> node = make_node<Evaluate>();
    node->value = v;
    return Stmt(node);
}

EXPORT Expr Call::make(Type type, std::string name, Array<Expr> args, CallType call_type,
                FunctionRef func, int value_index) {
    for (size_t i = 0; i < args.size(); i++) {
        internal_assert(args[i].defined()) << "Call of undefined\n";
    }
    if (call_type == Halide) {
        for (size_t i = 0; i < args.size(); i++) {
            internal_assert(args[i].type() == Int(32))
            << "Args to call to halide function must be type Int(32)\n";
        }
    }

    NodePtr<Call> node = make_node<Call>();
    node->type = type;
    node->name = std::move(name);
    node->args = std::move(args);
    node->call_type = call_type;
    node->func = std::move(func);
    node->value_index = value_index;
    return Expr(node);
}

VarExpr Variable::make(Type type, std::string name_hint) {
    NodePtr<Variable> node = make_node<Variable>();
    node->type = type;
    node->name_hint = std::move(name_hint);
    return VarExpr(node);
}

Expr Shuffle::make(Array<Expr> vectors,
                   Array<Expr> indices) {
    internal_assert(vectors.size() != 0) << "Shuffle of zero vectors.\n";
    internal_assert(indices.size() != 0) << "Shufle with zero indices.\n";
    Type element_ty = vectors[0].type().element_of();
    int input_lanes = 0;
    for (Expr i : vectors) {
        internal_assert(i.type().element_of() == element_ty) << "Shuffle of vectors of mismatched types.\n";
        input_lanes += i.type().lanes();
    }
    for (Expr i : indices) {
        const IntImm* v = i.as<IntImm>();
        internal_assert(v)
              << "Shuffle vector indices must be constant integer\n";
        internal_assert(0 <= v->value && v->value < input_lanes)
              << "Shuffle vector index out of range: " << i << "\n";
    }

    NodePtr<Shuffle> node = make_node<Shuffle>();
    node->type = element_ty.with_lanes((int)indices.size());
    node->vectors = std::move(vectors);
    node->indices = std::move(indices);
    return Expr(node);
}

Expr Shuffle::make_interleave(Array<Expr> vectors) {
    internal_assert(!vectors.empty()) << "Interleave of zero vectors.\n";

    if (vectors.size() == 1) {
        return vectors[0];
    }

    int lanes = vectors[0].type().lanes();

    for (Expr i : vectors) {
        internal_assert(i.type().lanes() == lanes)
            << "Interleave of vectors with different sizes.\n";
    }

    Array<Expr> indices;
    for (int i = 0; i < lanes; i++) {
        for (int j = 0; j < (int)vectors.size(); j++) {
            indices.push_back(IntImm::make(Int(32), j * lanes + i));
        }
    }

    return make(vectors, indices);
}

Expr Shuffle::make_concat(Array<Expr> vectors) {
    internal_assert(!vectors.empty()) << "Concat of zero vectors.\n";

    if (vectors.size() == 1) {
        return vectors[0];
    }

    Array<Expr> indices;
    int lane = 0;
    for (int i = 0; i < (int)vectors.size(); i++) {
        for (int j = 0; j < vectors[i].type().lanes(); j++) {
            indices.push_back(IntImm::make(Int(32), lane++));
        }
    }

    return make(vectors, indices);
}

Expr Shuffle::make_slice(Expr vector, int begin, int stride, int size) {
    if (begin == 0 && size == vector.type().lanes() && stride == 1) {
        return vector;
    }

    Array<Expr> indices;
    for (int i = 0; i < size; i++) {
        indices.push_back(IntImm::make(Int(32), begin + i * stride));
    }

    return make({vector}, indices);
}

Expr Shuffle::make_extract_element(Expr vector, int i) {
    return make_slice(vector, i, 1, 1);
}

bool Shuffle::is_interleave() const {
    int lanes = vectors[0].type().lanes();

    // Don't consider concat of scalars as an interleave.
    if (lanes == 1) {
        return false;
    }

    for (Expr i : vectors) {
        if (i.type().lanes() != lanes) {
            return false;
        }
    }

    // Require that we are a complete interleaving.
    if (lanes * vectors.size() != indices.size()) {
        return false;
    }

    for (int i = 0; i < (int)vectors.size(); i++) {
        for (int j = 0; j < lanes; j++) {
            if (indices[j * (int)vectors.size() + i].as<IntImm>()->value != i * lanes + j) {
                return false;
            }
        }
    }

    return true;
}

namespace {

// Helper function to determine if a sequence of indices is a
// contiguous ramp.
bool is_ramp(const Array<Expr> &indices, int stride = 1) {
    for (size_t i = 0; i + 1 < indices.size(); i++) {
        if (indices[i + 1].as<IntImm>()->value != indices[i].as<IntImm>()->value + stride) {
            return false;
        }
    }
    return true;
}

}  // namespace

bool Shuffle::is_concat() const {
    size_t input_lanes = 0;
    for (Expr i : vectors ) {
        input_lanes += i.type().lanes();
    }

    // A concat is a ramp where the output has the same number of
    // lanes as the input.
    return indices.size() == input_lanes && is_ramp(indices);
}

bool Shuffle::is_slice() const {
    size_t input_lanes = 0;
    for (Expr i : vectors) {
        input_lanes += i.type().lanes();
    }

    // A slice is a ramp where the output does not contain all of the
    // lanes of the input.
    return indices.size() < input_lanes && is_ramp(indices, slice_stride());
}

bool Shuffle::is_extract_element() const {
    return indices.size() == 1;
}

template<> EXPORT void ExprNode<IntImm>::accept(IRVisitor *v, const Expr &e) const { v->visit((const IntImm *)this, e); }
template<> EXPORT void ExprNode<UIntImm>::accept(IRVisitor *v, const Expr &e) const { v->visit((const UIntImm *)this, e); }
template<> EXPORT void ExprNode<FloatImm>::accept(IRVisitor *v, const Expr &e) const { v->visit((const FloatImm *)this, e); }
template<> EXPORT void ExprNode<StringImm>::accept(IRVisitor *v, const Expr &e) const { v->visit((const StringImm *)this, e); }
template<> EXPORT void ExprNode<Cast>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Cast *)this, e); }
template<> EXPORT void ExprNode<Variable>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Variable *)this, e); }
template<> EXPORT void ExprNode<Add>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Add *)this, e); }
template<> EXPORT void ExprNode<Sub>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Sub *)this, e); }
template<> EXPORT void ExprNode<Mul>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Mul *)this, e); }
template<> EXPORT void ExprNode<Div>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Div *)this, e); }
template<> EXPORT void ExprNode<Mod>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Mod *)this, e); }
template<> EXPORT void ExprNode<Min>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Min *)this, e); }
template<> EXPORT void ExprNode<Max>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Max *)this, e); }
template<> EXPORT void ExprNode<EQ>::accept(IRVisitor *v, const Expr &e) const { v->visit((const EQ *)this, e); }
template<> EXPORT void ExprNode<NE>::accept(IRVisitor *v, const Expr &e) const { v->visit((const NE *)this, e); }
template<> EXPORT void ExprNode<LT>::accept(IRVisitor *v, const Expr &e) const { v->visit((const LT *)this, e); }
template<> EXPORT void ExprNode<LE>::accept(IRVisitor *v, const Expr &e) const { v->visit((const LE *)this, e); }
template<> EXPORT void ExprNode<GT>::accept(IRVisitor *v, const Expr &e) const { v->visit((const GT *)this, e); }
template<> EXPORT void ExprNode<GE>::accept(IRVisitor *v, const Expr &e) const { v->visit((const GE *)this, e); }
template<> EXPORT void ExprNode<And>::accept(IRVisitor *v, const Expr &e) const { v->visit((const And *)this, e); }
template<> EXPORT void ExprNode<Or>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Or *)this, e); }
template<> EXPORT void ExprNode<Not>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Not *)this, e); }
template<> EXPORT void ExprNode<Select>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Select *)this, e); }
template<> EXPORT void ExprNode<Load>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Load *)this, e); }
template<> EXPORT void ExprNode<Ramp>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Ramp *)this, e); }
template<> EXPORT void ExprNode<Broadcast>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Broadcast *)this, e); }
template<> EXPORT void ExprNode<Call>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Call *)this, e); }
template<> EXPORT void ExprNode<Let>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Let *)this, e); }
template<> EXPORT void StmtNode<LetStmt>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const LetStmt *)this, s); }
template<> EXPORT void StmtNode<AttrStmt>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const AttrStmt *)this, s); }
template<> EXPORT void StmtNode<AssertStmt>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const AssertStmt *)this, s); }
template<> EXPORT void StmtNode<ProducerConsumer>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const ProducerConsumer *)this, s); }
template<> EXPORT void StmtNode<For>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const For *)this, s); }
template<> EXPORT void StmtNode<Store>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const Store *)this, s); }
template<> EXPORT void StmtNode<Provide>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const Provide *)this, s); }
template<> EXPORT void StmtNode<Allocate>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const Allocate *)this, s); }
template<> EXPORT void StmtNode<Free>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const Free *)this, s); }
template<> EXPORT void StmtNode<Realize>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const Realize *)this, s); }
template<> EXPORT void StmtNode<Prefetch>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const Prefetch *)this, s); }
template<> EXPORT void StmtNode<Block>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const Block *)this, s); }
template<> EXPORT void StmtNode<IfThenElse>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const IfThenElse *)this, s); }
template<> EXPORT void StmtNode<Evaluate>::accept(IRVisitor *v, const Stmt &s) const { v->visit((const Evaluate *)this, s); }
template<> EXPORT void ExprNode<Shuffle>::accept(IRVisitor *v, const Expr &e) const { v->visit((const Shuffle *)this, e); }

Call::ConstString Call::debug_to_file = "debug_to_file";
Call::ConstString Call::reinterpret = "reinterpret";
Call::ConstString Call::bitwise_and = "bitwise_and";
Call::ConstString Call::bitwise_not = "bitwise_not";
Call::ConstString Call::bitwise_xor = "bitwise_xor";
Call::ConstString Call::bitwise_or = "bitwise_or";
Call::ConstString Call::shift_left = "shift_left";
Call::ConstString Call::shift_right = "shift_right";
Call::ConstString Call::abs = "abs";
Call::ConstString Call::absd = "absd";
Call::ConstString Call::lerp = "lerp";
Call::ConstString Call::random = "random";
Call::ConstString Call::popcount = "popcount";
Call::ConstString Call::count_leading_zeros = "count_leading_zeros";
Call::ConstString Call::count_trailing_zeros = "count_trailing_zeros";
Call::ConstString Call::undef = "undef";
Call::ConstString Call::return_second = "return_second";
Call::ConstString Call::if_then_else = "if_then_else";
Call::ConstString Call::glsl_texture_load = "glsl_texture_load";
Call::ConstString Call::glsl_texture_store = "glsl_texture_store";
Call::ConstString Call::glsl_varying = "glsl_varying";
Call::ConstString Call::image_load = "image_load";
Call::ConstString Call::image_store = "image_store";
Call::ConstString Call::make_struct = "make_struct";
Call::ConstString Call::stringify = "stringify";
Call::ConstString Call::memoize_expr = "memoize_expr";
Call::ConstString Call::alloca = "alloca";
Call::ConstString Call::likely = "likely";
Call::ConstString Call::likely_if_innermost = "likely_if_innermost";
Call::ConstString Call::register_destructor = "register_destructor";
Call::ConstString Call::div_round_to_zero = "div_round_to_zero";
Call::ConstString Call::mod_round_to_zero = "mod_round_to_zero";
Call::ConstString Call::call_cached_indirect_function = "call_cached_indirect_function";
Call::ConstString Call::prefetch = "prefetch";
Call::ConstString Call::signed_integer_overflow = "signed_integer_overflow";
Call::ConstString Call::indeterminate_expression = "indeterminate_expression";
Call::ConstString Call::bool_to_mask = "bool_to_mask";
Call::ConstString Call::cast_mask = "cast_mask";
Call::ConstString Call::select_mask = "select_mask";
Call::ConstString Call::extract_mask_element = "extract_mask_element";
Call::ConstString Call::size_of_halideir_buffer_t = "size_of_halideir_buffer_t";
}
}

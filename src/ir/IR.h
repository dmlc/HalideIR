#ifndef HALIDE_IR_H
#define HALIDE_IR_H

/** \file
 * Subtypes for Halide expressions (\ref Halide::Expr) and statements (\ref Halide::Internal::Stmt)
 */

#include <string>
#include <vector>

#include "base/Debug.h"
#include "base/Error.h"
#include "base/Type.h"
#include "base/Util.h"
#include "Expr.h"

namespace Halide {
namespace Internal {

/** The actual IR nodes begin here. Remember that all the Expr
 * nodes also have a public "type" property */

/** Integer constants */
struct IntImm : public ExprNode<IntImm> {
    int64_t value;

    static Expr make(Type t, int64_t value);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("value", &value);
    }
    static const IRNodeType _type_info = IRNodeType::IntImm;
    static constexpr const char* _type_key = "IntImm";
};

/** Unsigned integer constants */
struct UIntImm : public ExprNode<UIntImm> {
    uint64_t value;

    static Expr make(Type t, uint64_t value);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("value", &value);
    }
    static const IRNodeType _type_info = IRNodeType::UIntImm;
    static constexpr const char* _type_key = "UIntImm";
};

/** Floating point constants */
struct FloatImm : public ExprNode<FloatImm> {
    double value;

  static Expr make(Type t, double value);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("value", &value);
    }
    static const IRNodeType _type_info = IRNodeType::FloatImm;
    static constexpr const char* _type_key = "FloatImm";
};

/** String constants */
struct StringImm : public ExprNode<StringImm> {
    std::string value;

    Expr static make(const std::string &val);
    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("value", &value);
    }
    static const IRNodeType _type_info = IRNodeType::StringImm;
    static constexpr const char* _type_key = "StringImm";
};

/** Cast a node from one type to another. Can't change vector widths. */
struct Cast : public ExprNode<Cast> {
    Expr value;

    EXPORT static Expr make(Type t, Expr v);
    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("value", &value);
    }
    static const IRNodeType _type_info = IRNodeType::Cast;
    static constexpr const char* _type_key = "Cast";
};

/** base class of all Binary arithematic ops */
template<typename T>
struct BinaryOpNode : public ExprNode<T> {
    Expr a, b;

    EXPORT static Expr make(Expr a, Expr b) {
       internal_assert(a.defined()) << "BinaryOp of undefined\n";
       internal_assert(b.defined()) << "BinaryOp of undefined\n";
       internal_assert(a.type() == b.type()) << "BinaryOp of mismatched types\n";
       std::shared_ptr<T> node = std::make_shared<T>();
       node->type = a.type();
       node->a = a;
       node->b = b;
       return Expr(node);
    }
    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &(this->type));
        v->Visit("a", &a);
        v->Visit("b", &b);
    }
};


/** The sum of two expressions */
struct Add : public BinaryOpNode<Add> {
    static const IRNodeType _type_info = IRNodeType::Add;
    static constexpr const char* _type_key = "Add";
};

/** The difference of two expressions */
struct Sub : public BinaryOpNode<Sub> {
    static const IRNodeType _type_info = IRNodeType::Sub;
    static constexpr const char* _type_key = "Sub";
};

/** The product of two expressions */
struct Mul : public BinaryOpNode<Mul> {
    static const IRNodeType _type_info = IRNodeType::Mul;
    static constexpr const char* _type_key = "Mul";
};

/** The ratio of two expressions */
struct Div : public BinaryOpNode<Div> {
    static const IRNodeType _type_info = IRNodeType::Div;
    static constexpr const char* _type_key = "Div";
};

/** The remainder of a / b. Mostly equivalent to '%' in C, except that
 * the result here is always positive. For floats, this is equivalent
 * to calling fmod. */
struct Mod : public BinaryOpNode<Mod> {
    static const IRNodeType _type_info = IRNodeType::Mod;
    static constexpr const char* _type_key = "Mod";
};

/** The lesser of two values. */
struct Min : public BinaryOpNode<Min> {
    static const IRNodeType _type_info = IRNodeType::Min;
    static constexpr const char* _type_key = "Min";
};

/** The greater of two values */
struct Max : public BinaryOpNode<Max> {
    static const IRNodeType _type_info = IRNodeType::Max;
    static constexpr const char* _type_key = "Max";
};

/** base class of all comparison ops */
template<typename T>
struct CmpOpNode : public ExprNode<T> {
    Expr a, b;

    EXPORT static Expr make(Expr a, Expr b) {
        internal_assert(a.defined()) << "CmpOp of undefined\n";
        internal_assert(b.defined()) << "CmpOp of undefined\n";
        internal_assert(a.type() == b.type()) << "BinaryOp of mismatched types\n";
        std::shared_ptr<T> node = std::make_shared<T>();
        node->type = Bool(a.type().lanes());
        node->a = a;
        node->b = b;
        return Expr(node);
    }
    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &(this->type));
        v->Visit("a", &a);
        v->Visit("b", &b);
    }
};

/** Is the first expression equal to the second */
struct EQ : public CmpOpNode<EQ> {
    static const IRNodeType _type_info = IRNodeType::EQ;
    static constexpr const char* _type_key = "EQ";
};

/** Is the first expression not equal to the second */
struct NE : public CmpOpNode<NE> {
    static const IRNodeType _type_info = IRNodeType::NE;
    static constexpr const char* _type_key = "NE";
};

/** Is the first expression less than the second. */
struct LT : public CmpOpNode<LT> {
    static const IRNodeType _type_info = IRNodeType::LT;
    static constexpr const char* _type_key = "LT";
};

/** Is the first expression less than or equal to the second. */
struct LE : public CmpOpNode<LE> {
    static const IRNodeType _type_info = IRNodeType::LE;
    static constexpr const char* _type_key = "LE";
};

/** Is the first expression greater than the second. */
struct GT : public CmpOpNode<GT> {
    static const IRNodeType _type_info = IRNodeType::GT;
    static constexpr const char* _type_key = "GT";
};

/** Is the first expression greater than or equal to the second. */
struct GE : public CmpOpNode<GE> {
    static const IRNodeType _type_info = IRNodeType::GE;
    static constexpr const char* _type_key = "GE";
};

/** Logical and - are both expressions true */
struct And : public ExprNode<And> {
    Expr a, b;

    EXPORT static Expr make(Expr a, Expr b);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &(this->type));
        v->Visit("a", &a);
        v->Visit("b", &b);
    }
    static const IRNodeType _type_info = IRNodeType::And;
    static constexpr const char* _type_key = "And";
};

/** Logical or - is at least one of the expression true */
struct Or : public ExprNode<Or> {
    Expr a, b;

    EXPORT static Expr make(Expr a, Expr b);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("a", &a);
        v->Visit("b", &b);
    }
    static const IRNodeType _type_info = IRNodeType::Or;
    static constexpr const char* _type_key = "Or";
};

/** Logical not - true if the expression false */
struct Not : public ExprNode<Not> {
    Expr a;

    EXPORT static Expr make(Expr a);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("a", &a);
    }
    static const IRNodeType _type_info = IRNodeType::Not;
    static constexpr const char* _type_key = "Not";
};

/** A ternary operator. Evalutes 'true_value' and 'false_value',
 * then selects between them based on 'condition'. Equivalent to
 * the ternary operator in C. */
struct Select : public ExprNode<Select> {
    Expr condition, true_value, false_value;

    EXPORT static Expr make(Expr condition, Expr true_value, Expr false_value);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("condition", &condition);
        v->Visit("true_value", &true_value);
        v->Visit("false_value", &false_value);
    }
    static const IRNodeType _type_info = IRNodeType::Select;
    static constexpr const char* _type_key = "Select";
};

/** Load a value from a named buffer. The buffer is treated as an
 * array of the 'type' of this Load node. That is, the buffer has
 * no inherent type. */
struct Load : public ExprNode<Load> {
    std::string name;
    Expr index;

    EXPORT static Expr make(Type type, std::string name, Expr index);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("name", &name);
        v->Visit("name", &index);
    }
    static const IRNodeType _type_info = IRNodeType::Load;
    static constexpr const char* _type_key = "Load";
};

/** A linear ramp vector node. This is vector with 'lanes' elements,
 * where element i is 'base' + i*'stride'. This is a convenient way to
 * pass around vectors without busting them up into individual
 * elements. E.g. a dense vector load from a buffer can use a ramp
 * node with stride 1 as the index. */
struct Ramp : public ExprNode<Ramp> {
    Expr base, stride;
    int lanes;

    EXPORT static Expr make(Expr base, Expr stride, int lanes);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("base", &base);
        v->Visit("stride", &stride);
        v->Visit("lanes", &lanes);
    }
    static const IRNodeType _type_info = IRNodeType::Ramp;
    static constexpr const char* _type_key = "Ramp";
};

/** A vector with 'lanes' elements, in which every element is
 * 'value'. This is a special case of the ramp node above, in which
 * the stride is zero. */
struct Broadcast : public ExprNode<Broadcast> {
    Expr value;
    int lanes;

    EXPORT static Expr make(Expr value, int lanes);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("value", &value);
        v->Visit("lanes", &lanes);
    }
    static const IRNodeType _type_info = IRNodeType::Broadcast;
    static constexpr const char* _type_key = "Broadcast";
};

/** A let expression, like you might find in a functional
 * language. Within the expression \ref Let::body, instances of the Var
 * node \ref Let::name refer to \ref Let::value. */
struct Let : public ExprNode<Let> {
    std::string name;
    Expr value, body;

    EXPORT static Expr make(std::string name, Expr value, Expr body);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("name", &name);
        v->Visit("value", &value);
        v->Visit("body", &body);
    }
    static const IRNodeType _type_info = IRNodeType::Let;
    static constexpr const char* _type_key = "Let";
};

/** The statement form of a let node. Within the statement 'body',
 * instances of the Var named 'name' refer to 'value' */
struct LetStmt : public StmtNode<LetStmt> {
    std::string name;
    Expr value;
    Stmt body;

    EXPORT static Stmt make(std::string name, Expr value, Stmt body);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("name", &name);
        v->Visit("value", &value);
        v->Visit("body", &body);
    }
    static const IRNodeType _type_info = IRNodeType::LetStmt;
    static constexpr const char* _type_key = "LetStmt";
};

/** If the 'condition' is false, then evaluate and return the message,
 * which should be a call to an error function. */
struct AssertStmt : public StmtNode<AssertStmt> {
    // if condition then val else error out with message
    Expr condition;
    Expr message;

    EXPORT static Stmt make(Expr condition, Expr message);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("condition", &condition);
        v->Visit("message", &message);
    }
    static const IRNodeType _type_info = IRNodeType::AssertStmt;
    static constexpr const char* _type_key = "AssertStmt";
};

/** This node is a helpful annotation to do with permissions. If 'is_produce' is
 * set to true, this represents a producer node which may also contain updates;
 * otherwise, this represents a consumer node. If the producer node contains
 * updates, the body of the node will be a block of 'produce' and 'update'
 * in that order. In a producer node, the access is read-write only (or write
 * only if it doesn't have updates). In a consumer node, the access is read-only.
 * None of this is actually enforced, the node is purely for informative purposes
 * to help out our analysis during lowering. For every unique ProducerConsumer,
 * there is an associated Realize node with the same name that creates the buffer
 * being read from or written to in the body of the ProducerConsumer.
 */
struct ProducerConsumer : public StmtNode<ProducerConsumer> {
    std::string name;
    bool is_producer;
    Stmt body;

    EXPORT static Stmt make(std::string name, bool is_producer, Stmt body);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("name", &name);
        v->Visit("is_producer", &is_producer);
        v->Visit("body", &body);
    }
    static const IRNodeType _type_info = IRNodeType::ProducerConsumer;
    static constexpr const char* _type_key = "ProducerConsumer";
};

/** Store a 'value' to the buffer called 'name' at a given
 * 'index'. The buffer is interpreted as an array of the same type as
 * 'value'. */
struct Store : public StmtNode<Store> {
    std::string name;
    Expr value, index;

    EXPORT static Stmt make(std::string name, Expr value, Expr index);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("name", &name);
        v->Visit("value", &value);
        v->Visit("index", &index);
    }
    static const IRNodeType _type_info = IRNodeType::Store;
    static constexpr const char* _type_key = "Store";
};

/** This defines the value of a function at a multi-dimensional
 * location. You should think of it as a store to a
 * multi-dimensional array. It gets lowered to a conventional
 * Store node. */
struct Provide : public StmtNode<Provide> {
    std::string name;
    std::vector<Expr> values;
    std::vector<Expr> args;

    EXPORT static Stmt make(std::string name, const std::vector<Expr> &values, const std::vector<Expr> &args);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("name", &name);
        // TODO(tqchen)
        // v->Visit("value", &value);
        // v->Visit("index", &index);
    }
    static const IRNodeType _type_info = IRNodeType::Provide;
    static constexpr const char* _type_key = "Provide";
};

/** Allocate a scratch area called with the given name, type, and
 * size. The buffer lives for at most the duration of the body
 * statement, within which it is freed. It is an error for an allocate
 * node not to contain a free node of the same buffer. Allocation only
 * occurs if the condition evaluates to true. */
struct Allocate : public StmtNode<Allocate> {
    std::string name;
    Type type;
    std::vector<Expr> extents;
    Expr condition;

    // These override the code generator dependent malloc and free
    // equivalents if provided. If the new_expr succeeds, that is it
    // returns non-nullptr, the function named be free_function is
    // guaranteed to be called. The free function signature must match
    // that of the code generator dependent free (typically
    // halide_free). If free_function is left empty, code generator
    // default will be called.
    Expr new_expr;
    std::string free_function;
    Stmt body;

    EXPORT static Stmt make(std::string name, Type type, const std::vector<Expr> &extents,
                            Expr condition, Stmt body,
                            Expr new_expr = Expr(), std::string free_function = std::string());

    /** A routine to check if the extents are all constants, and if so verify
     * the total size is less than 2^31 - 1. If the result is constant, but
     * overflows, this routine asserts. This returns 0 if the extents are
     * not all constants; otherwise, it returns the total constant allocation
     * size. */
    EXPORT static int32_t constant_allocation_size(const std::vector<Expr> &extents, const std::string &name);
    EXPORT int32_t constant_allocation_size() const;

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("name", &name);
        v->Visit("type", &type);
        // TODO(tqchen)
        // v->Visit("extents", &extents);
        v->Visit("condition", &condition);
    }
    static const IRNodeType _type_info = IRNodeType::Allocate;
    static constexpr const char* _type_key = "Allocate";
};

/** Free the resources associated with the given buffer. */
struct Free : public StmtNode<Free> {
    std::string name;

    EXPORT static Stmt make(std::string name);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("name", &name);
    }
    static const IRNodeType _type_info = IRNodeType::Free;
    static constexpr const char* _type_key = "Free";
};

/** A single-dimensional span. Includes all numbers between min and
 * (min + extent - 1) */
struct Range {
    Expr min, extent;
    Range() {}
    Range(Expr min, Expr extent) : min(min), extent(extent) {
        internal_assert(min.type() == extent.type()) << "Region min and extent must have same type\n";
    }
};

/** A multi-dimensional box. The outer product of the elements */
typedef std::vector<Range> Region;

/** Allocate a multi-dimensional buffer of the given type and
 * size. Create some scratch memory that will back the function 'name'
 * over the range specified in 'bounds'. The bounds are a vector of
 * (min, extent) pairs for each dimension. Allocation only occurs if
 * the condition evaluates to true. */
struct Realize : public StmtNode<Realize> {
    std::string name;
    std::vector<Type> types;
    Region bounds;
    Expr condition;
    Stmt body;

    EXPORT static Stmt make(const std::string &name, const std::vector<Type> &types, const Region &bounds, Expr condition, Stmt body);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("name", &name);
        // v->Visit("types", &types);
        // v->Visit("bounds", &bounds);
        v->Visit("condition", &condition);
        v->Visit("body", &body);
    }
    static const IRNodeType _type_info = IRNodeType::Realize;
    static constexpr const char* _type_key = "Realize";
};

/** A sequence of statements to be executed in-order. 'rest' may be
 * undefined. Used rest.defined() to find out. */
struct Block : public StmtNode<Block> {
    Stmt first, rest;

    EXPORT static Stmt make(Stmt first, Stmt rest);
    EXPORT static Stmt make(const std::vector<Stmt> &stmts);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("first", &first);
        v->Visit("rest", &rest);
    }
    static const IRNodeType _type_info = IRNodeType::Block;
    static constexpr const char* _type_key = "Block";
};

/** An if-then-else block. 'else' may be undefined. */
struct IfThenElse : public StmtNode<IfThenElse> {
    Expr condition;
    Stmt then_case, else_case;

    EXPORT static Stmt make(Expr condition, Stmt then_case, Stmt else_case = Stmt());

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("condition", &condition);
        v->Visit("then_case", &then_case);
        v->Visit("else_case", &else_case);
    }
    static const IRNodeType _type_info = IRNodeType::IfThenElse;
    static constexpr const char* _type_key = "IfThenElse";
};

/** Evaluate and discard an expression, presumably because it has some side-effect. */
struct Evaluate : public StmtNode<Evaluate> {
    Expr value;

    EXPORT static Stmt make(Expr v);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("value", &value);
    }
    static const IRNodeType _type_info = IRNodeType::Evaluate;
    static constexpr const char* _type_key = "Evaluate";
};

/** A function call. This can represent a call to some extern function
 * (like sin), but it's also our multi-dimensional version of a Load,
 * so it can be a load from an input image, or a call to another
 * halide function. These two types of call nodes don't survive all
 * the way down to code generation - the lowering process converts
 * them to Load nodes. */
struct Call : public ExprNode<Call> {
    std::string name;
    std::vector<Expr> args;
    enum CallType : int {
      Extern,       //< A call to an external C-ABI function, possibly with side-effects
      ExternCPlusPlus, //< A call to an external C-ABI function, possibly with side-effects
      PureExtern,   //< A call to a guaranteed-side-effect-free external function
      Halide,       //< A call to a Func
      Intrinsic,    //< A possibly-side-effecty compiler intrinsic, which has special handling during codegen
      PureIntrinsic //< A side-effect-free version of the above.
    };
    CallType call_type;

    // Halide uses calls internally to represent certain operations
    // (instead of IR nodes). These are matched by name. Note that
    // these are deliberately char* (rather than std::string) so that
    // they can be referenced at static-initialization time without
    // risking ambiguous initalization order; we use a typedef to simplify
    // declaration.
    typedef const char* const ConstString;

    EXPORT static ConstString debug_to_file,
        shuffle_vector,
        interleave_vectors,
        concat_vectors,
        reinterpret,
        bitwise_and,
        bitwise_not,
        bitwise_xor,
        bitwise_or,
        shift_left,
        shift_right,
        abs,
        absd,
        rewrite_buffer,
        random,
        lerp,
        create_buffer_t,
        copy_buffer_t,
        extract_buffer_min,
        extract_buffer_max,
        extract_buffer_host,
        set_host_dirty,
        set_dev_dirty,
        popcount,
        count_leading_zeros,
        count_trailing_zeros,
        undef,
        null_handle,
        address_of,
        return_second,
        if_then_else,
        trace,
        trace_expr,
        glsl_texture_load,
        glsl_texture_store,
        glsl_varying,
        image_load,
        image_store,
        make_struct,
        stringify,
        memoize_expr,
        copy_memory,
        likely,
        likely_if_innermost,
        register_destructor,
        div_round_to_zero,
        mod_round_to_zero,
        slice_vector,
        call_cached_indirect_function,
        prefetch,
        prefetch_2d,
        signed_integer_overflow,
        indeterminate_expression,
        bool_to_mask,
        cast_mask,
        select_mask;

    // If it's a call to another halide function, this call node holds
    // onto a pointer to that function for the purposes of reference
    // counting only. Self-references in update definitions do not
    // have this set, to avoid cycles.
    std::shared_ptr<const IRNode> func;

    // If that function has multiple values, which value does this
    // call node refer to?
    int value_index{0};

    EXPORT static Expr make(Type type, std::string name,
                            const std::vector<Expr> &args,
                            CallType call_type,
                            std::shared_ptr<const IRNode> func = nullptr,
                            int value_index = 0);

    /** Check if a call node is pure within a pipeline, meaning that
     * the same args always give the same result, and the calls can be
     * reordered, duplicated, unified, etc without changing the
     * meaning of anything. Not transitive - doesn't guarantee the
     * args themselves are pure. An example of a pure Call node is
     * sqrt. If in doubt, don't mark a Call node as pure. */
    bool is_pure() const {
        return (call_type == PureExtern ||
                call_type == PureIntrinsic);
    }

    bool is_intrinsic(ConstString intrin_name) const {
        return
            ((call_type == Intrinsic ||
              call_type == PureIntrinsic) &&
             name == intrin_name);
    }

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("type", &type);
        v->Visit("name", &name);
        // TODO(tqchen)
        // v->Visit("args", &args);
        v->Visit("call_type", &call_type);
        // v->Visit("func", &func);
        v->Visit("value_index", &value_index);
    }
    static const IRNodeType _type_info = IRNodeType::Call;
    static constexpr const char* _type_key = "Call";
};

/** A named variable. Might be a loop variable, function argument,
 * parameter, reduction variable, or something defined by a Let or
 * LetStmt node. */
struct Variable : public ExprNode<Variable> {
    /**
     * variable is uniquely identified by its address instead of name
     * This field is renamed to name_hint to make it different from
     * original ref by name convention
     */
    std::string name_hint;

    // BufferPtr and Parameter are removed from IR
    // They can be added back via passing in binding of Variable to specific values.
    // in the final stage of code generation.

    // refing back ReductionVariable from Variable can cause cyclic refs,
    // remove reference to reduction domain here,
    // instead,uses Reduction as a ExprNode

    EXPORT static Expr make(Type type, std::string name_hint);

    static const IRNodeType _type_info = IRNodeType::Variable;
    static constexpr const char* _type_key = "Variable";
};

/** A for loop. Execute the 'body' statement for all values of the
 * variable 'name' from 'min' to 'min + extent'. There are four
 * types of For nodes. A 'Serial' for loop is a conventional
 * one. In a 'Parallel' for loop, each iteration of the loop
 * happens in parallel or in some unspecified order. In a
 * 'Vectorized' for loop, each iteration maps to one SIMD lane,
 * and the whole loop is executed in one shot. For this case,
 * 'extent' must be some small integer constant (probably 4, 8, or
 * 16). An 'Unrolled' for loop compiles to a completely unrolled
 * version of the loop. Each iteration becomes its own
 * statement. Again in this case, 'extent' should be a small
 * integer constant. */
struct For : public StmtNode<For> {
    std::string name;
    Expr min, extent;
    ForType for_type;
    DeviceAPI device_api;
    Stmt body;

    EXPORT static Stmt make(std::string name, Expr min, Expr extent, ForType for_type, DeviceAPI device_api, Stmt body);

    void VisitAttrs(IR::AttrVisitor* v) final {
        v->Visit("name", &name);
        v->Visit("min", &min);
        v->Visit("extent", &extent);
        v->Visit("for_type", &for_type);
        v->Visit("device_api", &device_api);
        v->Visit("body", &body);
    }
    static const IRNodeType _type_info = IRNodeType::For;
    static constexpr const char* _type_key = "For";
};

}
}

#endif

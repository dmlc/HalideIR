/*!
 *  Copyright (c) 2016 by Contributors
 * \file node.h
 * \brief Defines the Node data structures.
 */
#ifndef TVM_IR_NODE_H_
#define TVM_IR_NODE_H_

#include <string>
#include <vector>
#include <type_traits>
#include "base/Debug.h"
#include "./node.h"

namespace tvm {

// This header enables for IRNode without dependency on RTTI.
// Each subclass IRNode will have a unique type_index
// This type index is determined at runtime given unique type_key of each IRNode
// The type index is ganranteed to be continuous, and unique, but can vary during runtime.

/*!
 * \brief the base class of all IRNode,
 *  IRNode have a runtime specific type index used for dispatching.
 */
class IRNode : public Node {
 public:
  /*!
   * \brief get a runtime unique type index given a type key
   * \param type_key Type key of a type.
   * \return the corresponding type index.
   */
  static uint32_t TypeKey2Index(const char* type_key);
  /*!
   * \brief get type key from type index.
   * \param index The type index
   * \return the corresponding type key.
   */
  static const char* TypeIndex2Key(uint32_t index);

 protected:
  friend class IRNodeRef;
  /*!
   * \brief runtime dependent type index that
   *  is unique to each IRNode type.
   *  0 means unknown type index.
   */
  uint32_t type_index_{0};
};

/*! \brief Base class of IRNode reference class */
class IRNodeRef : public NodeRef {
 public:
  /*! \return the internal type index of IRNode */
  inline uint32_t type_index() const {
    return static_cast<const IRNode*>(node_.get())->type_index_;
  }
  /*!
   * \brief Downcast this ir node to its actual type (e.g. Add, or
   * Select). This returns nullptr if the node is not of the requested
   * type. Example usage:
   *
   * if (const Add *add = node->as<Add>()) {
   *   // This is an add node
   * }
   * \tparam T the target type, must be subtype of IRNode
   */
  template<typename T> const T *as() const {
    const IRNode* ptr = static_cast<const IRNode*>(node_.get());
    // use static field so query only happens once.
    static uint32_t type_id = IRNode::TypeKey2Index(T::_type_key);
    if (ptr && ptr->type_index_ == type_id) {
      return static_cast<const T*>(ptr);
    }
    return nullptr;
  }

 protected:
  template<typename>
  friend class IRFunctor;
  IRNodeRef() = default;
  explicit IRNodeRef(std::shared_ptr<Node> node) : NodeRef(node) {}
};

/*!
 * \brief A dynamical dispatched functor on IRNodeRef in the first argument.
 *
 * \code
 *   IRFunctor<std::string (const IRNodeRef& n, std::string prefix)> tostr;
 *   tostr.set_dispatch<Add>([](const Add* op, std::string prefix) {
 *     return prefix + "Add";
 *   });
 *   tostr.set_dispatch<IntImm>([](const IntImm* op) {
 *     return prefix + "IntImm"
 *   });
 *
 *   Expr x = make_const(1);
 *   Expr y = x + x;
 *   // dispatch to IntImm, outputs "MyIntImm"
 *   LOG(INFO) << tostr(x, "My");
 *   // dispatch to IntImm, outputs "MyAdd"
 *   LOG(INFO) << tostr(y, "My");
 * \endcode
 *
 * \tparam FType function signiture
 *  This type if only definef for FType with function signiture
 */
template<typename FType>
class IRFunctor;

template<typename R, typename ...Args>
class IRFunctor<R(const IRNodeRef& n, Args...)> {
 private:
  using Function = std::function<R (const IRNodeRef&n, Args...)>;
  using TSelf = IRFunctor<R (const IRNodeRef& n, Args...)>;
  /*! \brief internal function table */
  std::vector<Function> func_;

 public:
  /*! \brief the result type of this functor */
  using result_type = R;
  /*!
   * \brief invoke the functor , dispatch on type of n
   * \param n The IRNode argument
   * \param args The additional arguments
   * \return The result.
   */
  inline R operator()(const IRNodeRef& n, Args... args) const {
    uint32_t type_index = n.type_index();
    internal_assert(type_index < func_.size() &&
                    func_[type_index] != nullptr)
        << "IRFunctor calls un-registered function on type"
        << IRNode::TypeIndex2Key(type_index);
    return func_[type_index](n, std::forward<Args>(args)...);
  }
  /*!
   * \brief set the dispacher for type TNode
   * \param f The function to be set.
   * \tparam TNode the type of IRNode to be dispatched.
   * \return reference to self.
   */
  template<typename TNode>
  inline TSelf& set_dispatch(Function f) {  // NOLINT(*)
    uint32_t tindex = IRNode::TypeKey2Index(TNode::_type_key);
    if (func_.size() <= tindex) {
      func_.resize(tindex + 1, nullptr);
    }
    internal_assert(func_[tindex] == nullptr)
        << "Dispatch for " << IRNode::TypeIndex2Key(tindex)
        << "is already set";
    func_[tindex] = f;
    return *this;
  }
  /*!
   * \brief set the dispacher for type TNode
   *  This allows f to used detailed const IRNode pointer to replace IRNodeRef
   *
   * \param f The function to be set.
   * \tparam TNode the type of IRNode to be dispatched.
   * \return reference to self.
   */
  template<typename TNode>
  inline TSelf& set_dispatch(std::function<R(const TNode* n, Args...)> f) { // NOLINT(*)
    Function fun = [f](const IRNodeRef& n, Args... args) {
      return f(static_cast<const TNode*>(n.node_.get()),
               std::forward<Args>(args)...);
    };
    return this->set_dispatch<TNode>(fun);
  }
};

#if defined(__GNUC__)
#define TVM_ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define TVM_ATTRIBUTE_UNUSED
#endif

/*!
 * \brief Useful macro to set IRFunctor dispatch in a global static field.
 *
 * \code
 *  // Use IRFunctor to implement IRPrinter similar to Visitor Pattern.
 *  // vtable allows easy patch in of new IRNode types, without changing
 *  // interface of IRPrinter.
 *
 *  class IRPrinter {
 *   public:
 *    std::ostream& stream;
 *    // the dispatch function.
 *    void print(Expr e) {
 *      const static FType& f = *vtable();
 *      f(e, this);
 *    }
 *
 *    using FType = IRFunctor<void (const IRNodeRef&, IRPrinter *)>;
 *    // function to return global function table
 *    static FType& vtable();
 *  };
 *
 *  // in cpp/cc file
 *  IRPrinter::FType& IRPrinter::vtable() { // NOLINT(*0
 *    static FType inst; return inst;
 *  }
 *
 *  TVM_STATIC_IR_FUNCTOR(IRPrinter, vtable)
 *  .set_dispatch<Add>([](const Add* n, IRPrinter* p) {
 *    p->print(n->a);
 *    p->stream << '+'
 *    p->print(n->b);
 *  });
 *
 *
 * \endcode
 *
 * \param ClsName The name of the class
 * \param FField The static function that returns a singleton of IRFunctor.
 */
#define TVM_STATIC_IR_FUNCTOR(ClsName, FField)                       \
  static auto& TVM_ATTRIBUTE_UNUSED __make_ ## ClsName ## FField  =  \
                              ClsName::FField()

}  // namespace tvm

#endif  // TVM_IR_NODE_H_

/*!
 *  Copyright (c) 2016 by Contributors
 * \file ir_functor.h
 * \brief Defines the IRFunctor data structures.
 */
#ifndef TVM_IR_FUNCTOR_H_
#define TVM_IR_FUNCTOR_H_

#include <string>
#include <vector>
#include <type_traits>
#include <functional>
#include "base/Debug.h"
#include "./node.h"

namespace tvm {
/*!
 * \brief A dynamical dispatched functor on NodeRef in the first argument.
 *
 * \code
 *   IRFunctor<std::string (const NodeRef& n, std::string prefix)> tostr;
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
 *  This type if only defined for FType with function signiture
 */
template<typename FType>
class IRFunctor;

template<typename R, typename ...Args>
class IRFunctor<R(const NodeRef& n, Args...)> {
 private:
  using Function = std::function<R (const NodeRef&n, Args...)>;
  using TSelf = IRFunctor<R (const NodeRef& n, Args...)>;
  /*! \brief internal function table */
  std::vector<Function> func_;

 public:
  /*! \brief the result type of this functor */
  using result_type = R;
  /*!
   * \brief Whether the functor can dispatch the corresponding Node
   * \param n The node to be dispatched
   * \return Whether dispatching function is registered for n's type.
   */
  inline bool can_dispatch(const NodeRef& n) const {
    uint32_t type_index = n.type_index();
    return type_index < func_.size() && func_[type_index] != nullptr;
  }
  /*!
   * \brief invoke the functor , dispatch on type of n
   * \param n The Node argument
   * \param args The additional arguments
   * \return The result.
   */
  inline R operator()(const NodeRef& n, Args... args) const {
    uint32_t type_index = n.type_index();
    internal_assert(type_index < func_.size() &&
                    func_[type_index] != nullptr)
        << "IRFunctor calls un-registered function on type "
        << Node::TypeIndex2Key(type_index);
    return func_[type_index](n, std::forward<Args>(args)...);
  }
  /*!
   * \brief set the dispacher for type TNode
   * \param f The function to be set.
   * \tparam TNode the type of Node to be dispatched.
   * \return reference to self.
   */
  template<typename TNode>
  inline TSelf& set_dispatch(Function f) {  // NOLINT(*)
    uint32_t tindex = Node::TypeKey2Index(TNode::_type_key);
    if (func_.size() <= tindex) {
      func_.resize(tindex + 1, nullptr);
    }
    internal_assert(func_[tindex] == nullptr)
        << "Dispatch for " << Node::TypeIndex2Key(tindex)
        << " is already set";
    func_[tindex] = f;
    return *this;
  }
  /*!
   * \brief set the dispacher for type TNode
   *  This allows f to used detailed const Node pointer to replace NodeRef
   *
   * \param f The function to be set.
   * \tparam TNode the type of Node to be dispatched.
   * \return reference to self.
   */
  template<typename TNode>
  inline TSelf& set_dispatch(std::function<R(const TNode* n, Args...)> f) { // NOLINT(*)
    Function fun = [f](const NodeRef& n, Args... args) {
      return f(static_cast<const TNode*>(n.node_.get()),
               std::forward<Args>(args)...);
    };
    return this->set_dispatch<TNode>(fun);
  }
  /*!
  * \brief unset the dispacher for type TNode
  *
  * \tparam TNode the type of Node to be dispatched.
  * \return reference to self.
  */
  template<typename TNode>
  inline TSelf& clear_dispatch() {  // NOLINT(*)
    uint32_t tindex = Node::TypeKey2Index(TNode::_type_key);
    if (func_.size() <= tindex) {
      func_.resize(tindex + 1, nullptr);
      //func_.resize(tindex + 1, std::shared_ptr<Function>());
    }
    func_[tindex] = nullptr;
    return *this;
  }
};

#if defined(__GNUC__)
#define TVM_ATTRIBUTE_UNUSED __attribute__((unused))
#else
#define TVM_ATTRIBUTE_UNUSED
#endif

/*! \brief helper macro to generate string concat */
#define TVM_STR_CONCAT_(__x, __y) __x##__y
#define TVM_STR_CONCAT(__x, __y) TVM_STR_CONCAT_(__x, __y)

#define TVM_REGISTER_VAR_DEF(ClsName)                                 \
  static TVM_ATTRIBUTE_UNUSED auto & __make_functor ## _ ## ClsName

/*!
 * \brief Useful macro to set IRFunctor dispatch in a global static field.
 *
 * \code
 *  // Use IRFunctor to implement IRPrinter similar to Visitor Pattern.
 *  // vtable allows easy patch in of new Node types, without changing
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
 *    using FType = IRFunctor<void (const NodeRef&, IRPrinter *)>;
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
  TVM_STR_CONCAT(TVM_REGISTER_VAR_DEF(ClsName), __COUNTER__)  =      \
                              ClsName::FField()

 /*!
 * \brief A container for a list of callbacks. All callbacks are invoked when
 * the object is destructed.
 */
class FreeList {
private:
  std::vector<std::function<void()>> free_list;

public:
  ~FreeList() {
    for (auto &f : free_list) {
      f();
    }
  }

  void append(std::function<void()> func) {
    free_list.push_back(func);
  }
};

/*!
* \brief A wrapper around IRFunctor that will record calls to set_dispatch
* and make a corresponding call to clear_dispatch when the last copy of
* the IRFunctorWrapper is destructed. When assigned to a static variable,
* this can be used by NNVM and other libraries to unregister callbacks when
* the library is unloaded. This prevents crashes when the underlying IRFunctor
* is destructed as it will no longer contain std::function instances allocated
* by a library that has been unloaded.
*/
template<typename FType>
class IRFunctorWrapper;

template<typename R, typename ...Args>
class IRFunctorWrapper<R(const NodeRef& n, Args...)> {
private:
  IRFunctor<R(const NodeRef& n, Args...)> *irf_;
  std::shared_ptr<FreeList> free_list;

  using TSelf = IRFunctorWrapper<R(const NodeRef& n, Args...)>;

public:
  IRFunctorWrapper(IRFunctor<R(const NodeRef& n, Args...)> *irf) {
    irf_ = irf;
    free_list = std::shared_ptr<FreeList>(new FreeList());
  }

  template<typename TNode>
  inline TSelf& set_dispatch(std::function<R(const TNode* n, Args...)> f) {  // NOLINT(*)
    irf_->set_dispatch<TNode>(f);
    auto irfCopy = irf_;
    free_list.get()->append([irfCopy] {
      irfCopy->clear_dispatch<TNode>();
      });
    return *this;
  }
};

/*!
* \brief Helper function for constructing an IRFunctorWrapper. This allows
* the compiler to deduce the template types.
*/
template<typename R, typename ...Args>
IRFunctorWrapper<R(const NodeRef& n, Args...)> make_irfunctor_wrapper(
  IRFunctor<R(const NodeRef& n, Args...)> *irf) {
  return IRFunctorWrapper<R(const NodeRef& n, Args...)>(irf);
}

#define TVM_AUTO_REGISTER_VAR_DEF(ClsName)                           \
  static TVM_ATTRIBUTE_UNUSED auto __make_functor ## _ ## ClsName

/*!
* \brief Macro to set IRFunctor dispatch in a global static field using an IRFunctorWrapper.
* Usage is exactly the same as TVM_STATIC_IR_FUNCTOR. Libraries should use this instead of
* TVM_STATIC_IR_FUNCTOR.
*/
#define TVM_AUTO_STATIC_IR_FUNCTOR(ClsName, FField)                  \
  TVM_STR_CONCAT(TVM_AUTO_REGISTER_VAR_DEF(ClsName), __COUNTER__)  = \
                        make_irfunctor_wrapper(&ClsName::FField())

}  // namespace tvm

#endif  // TVM_IR_FUNCTOR_H_

/*!
 *  Copyright (c) 2016 by Contributors
 * \file array.h
 * \brief Array container in the DSL graph.
 */
#ifndef TVM_ARRAY_H_
#define TVM_ARRAY_H_

#include <type_traits>
#include <vector>
#include <initializer_list>

#include "./node.h"

namespace tvm {

/*! \brief node content in array */
class ArrayNode : public Node {
 public:
  /*! \brief the data content */
  std::vector<std::shared_ptr<Node> > data;
  const char* type_key() const override {
    return "Array";
  }
  void VisitAttrs(AttrVisitor* visitor) final {
     // Visitor to array have no effect.
  }
};

/*!
 * \brief Array container of NodeRef in DSL graph.
 *  Array implements copy on write semantics, which means array is mutable
 *  but copy will happen when array is referenced in more than two places.
 *
 * operator[] only provide const acces, use Set to mutate the content.
 * \tparam T The content NodeRef type.
 */
template<typename T,
         typename = typename std::enable_if<std::is_base_of<NodeRef, T>::value>::type >
class Array : public NodeRef {
 public:
  /*!
   * \brief default constructor
   */
  Array() {}
  /*!
   * \brief move constructor
   * \param other source
   */
  Array(Array<T> && other) {  // NOLINT(*)
    node_ = std::move(other.node_);
  }
  /*!
   * \brief copy constructor
   * \param other source
   */
  Array(const Array<T> &other) { // NOLINT(*)
    node_ = other.node_;
  }
  /*!
   * \brief constructor from iterator
   * \param begin begin of iterator
   * \param end end of iterator
   * \tparam IterType The type of iterator
   */
  template<typename IterType>
  Array(IterType begin, IterType end) {
    assign(begin, end);
  }
  /*!
   * \brief constructor from initializer list
   * \param init The initalizer list
   */
  Array(std::initializer_list<T> init) { // NOLINT(*)
    assign(init.begin(), init.end());
  }
  /*!
   * \brief constructor from vector
   * \param init The vector
   */
  Array(const std::vector<T>& init) { // NOLINT(*)
    assign(init.begin(), init.end());
  }
  /*!
   * \brief move assign operator
   * \param other The source of assignment
   * \return reference to self.
   */
  Array<T>& operator=(Array<T> && other) {
    node_ = std::move(other.node_);
    return *this;
  }
  /*!
   * \brief copy assign operator
   * \param other The source of assignment
   * \return reference to self.
   */
  Array<T>& operator=(const Array<T> & other) {
    node_ = std::move(other.node_);
    return *this;
  }
  /*!
   * \brief reset the array to content from iterator.
   * \param begin begin of iterator
   * \param end end of iterator
   * \tparam IterType The type of iterator
   */
  template<typename IterType>
  void assign(IterType begin, IterType end) {
    auto n = std::make_shared<ArrayNode>();
    n->data.reserve(end - begin);
    for (IterType i = begin; i < end; ++i) {
      n->data.push_back(i->node_);
    }
    node_ = std::move(n);
  }
  /*!
   * \brief Read i-th element from array.
   * \param i The index
   * \return the i-th element.
   */
  inline T operator[](size_t i) const {
    T inst;
    inst.node_ = static_cast<const ArrayNode*>(node_.get())->data[i];
    return inst;
  }
  /*! \return The size of the array */
  inline size_t size() const {
    if (node_.get() == nullptr) return 0;
    return static_cast<const ArrayNode*>(node_.get())->data.size();
  }
  /*! \brief copy on write semantics */
  inline void CopyOnWrite() {
    if (node_.get() == nullptr || node_.unique()) return;
    node_ = std::make_shared<ArrayNode>(
        *static_cast<const ArrayNode*>(node_.get()));
  }
  /*!
   * \brief push a new item to the back of the list
   * \param item The item to be pushed.
   */
  inline void push_back(const T& item) {
    this->CopyOnWrite();
    static_cast<ArrayNode*>(node_.get())->data.push_back(item.node_);
  }
  /*!
   * \brief set i-th element of the array.
   * \param i The index
   * \param value The value to be setted.
   */
  inline void Set(size_t i, const T& value) {
    this->CopyOnWrite();
    static_cast<ArrayNode*>(node_.get())->data[i] = value.node_;
  }
  /*! \return whether array is empty */
  inline bool empty() const {
    return size() == 0;
  }
  friend std::ostream& operator<<(std::ostream &os, const Array<T>& r) {  // NOLINT(*)
    for (size_t i = 0; i < r.size(); ++i) {
      if (i == 0) {
        os << '[';
      } else {
        os << ", ";
      }
      os << r[i];
    }
    os << ']';
    return os;
  }
};

}  // namespace tvm

namespace Halide {
namespace IR {

using tvm::Array;

}  // namespace IR
}  // namespace Halide

#endif  // TVM_ARRAY_H_

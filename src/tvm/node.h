/*!
 *  Copyright (c) 2016 by Contributors
 * \file node.h
 * \brief Defines the Node data structures.
 */
#ifndef TVM_NODE_H_
#define TVM_NODE_H_

#include <string>
#include <vector>
#include <memory>
#include <type_traits>
#include "base/Type.h"

/** namespace of new IR ocde */
namespace tvm {

using Halide::Type;
// forward declaration
class Node;
class NodeRef;

/*!
 * \brief Visitor class to each node content.
 *  The content is going to be called for each field.
 */
class AttrVisitor {
 public:
//! \cond Doxygen_Suppress
  virtual void Visit(const char* key, double* value) = 0;
  virtual void Visit(const char* key, int64_t* value) = 0;
  virtual void Visit(const char* key, uint64_t* value) = 0;
  virtual void Visit(const char* key, int* value) = 0;
  virtual void Visit(const char* key, bool* value) = 0;
  virtual void Visit(const char* key, std::string* value) = 0;
  virtual void Visit(const char* key, Type* value) = 0;
  virtual void Visit(const char* key, NodeRef* value) = 0;
  template<typename ENum,
           typename = typename std::enable_if<std::is_enum<ENum>::value>::type>
  void Visit(const char* key, ENum* ptr) {
    static_assert(std::is_same<int, typename std::underlying_type<ENum>::type>::value,
                  "declare enum to be enum int to use visitor");
    this->Visit(key, reinterpret_cast<int*>(ptr));
  }
//! \endcond
};

/*!
 * \brief base class of node container in DSL AST.
 *  All object's internal is stored as std::shared_ptr<Node>
 */
class Node {
 public:
  /*! \brief virtual destructor */
  virtual ~Node() {}
  /*! \return The unique type key of the node */
  virtual const char* type_key() const = 0;
  /*!
   * \brief Apply visitor to each field of the Node
   *  Visitor could mutate the content of the node.
   *  override if Node contains attribute fields.
   * \param visitor The visitor
   */
  virtual void VisitAttrs(AttrVisitor* visitor) {}
  /*!
   * \tparam NodeType the type to be checked.
   *  Relies on RTTI
   * \return whether the stored type is node type
   */
  template<typename TNode>
  inline bool is_type() const;

  // node ref can see this
  friend class NodeRef;
  /*!
   * \brief optional: safe destruction function
   *  Can be called in destructor of composite types.
   *  This can be used to avoid stack overflow when
   *  recursive destruction long graph(1M nodes),
   *
   *  It is totally OK to not call this in destructor.
   */
  void Destroy();
};

/*! \brief base class of all node reference object */
class NodeRef {
 public:
  /*!
   * \return typed pointer of the node
   * \tparam TNode the type of the node.
   */
  template<typename TNode>
  inline const TNode* Get() const;
  /*! \return wheyjer the expression is null */
  inline bool defined() const;
  /*!
   * \brief Comparator
   * \param other Another node ref.
   * \return the compare result.
   */
  inline bool operator==(const NodeRef& other) const;
  /*!
   * \brief Comparator
   * \param other Another node ref.
   * \return the compare result.
   */
  inline bool same_as(const NodeRef& other) const;
  /*!
   * \brief Comparator
   * \param other Another node ref.
   * \return the compare result.
   */
  inline bool operator<(const NodeRef& other) const;
  /*!
   * \brief Comparator
   * \param other Another node ref.
   * \return the compare result.
   */
  inline bool operator!=(const NodeRef& other) const;
  /*! \return the hash function for NodeRef */
  inline size_t hash() const;
  /*!
   * \brief reset the internal node pointer
   * \param node The node pointer to be reseted.
   */
  inline void reset(std::shared_ptr<Node> node);

 protected:
  template<typename T, typename>
  friend class Array;
  friend class Node;
  friend class APIVariantValue;
  NodeRef() = default;
  explicit NodeRef(std::shared_ptr<Node> node) : node_(node) {}
  /*! \brief the internal node object, do not touch  */
  std::shared_ptr<Node> node_;
};


// implementations of inline functions after this
template<typename TNode>
inline bool Node::is_type() const {
  const std::type_info& tinfo = typeid(*this);
  if (&typeid(TNode) == &tinfo) return true;
  return typeid(TNode) == tinfo;
}

template<typename TNode>
inline const TNode* NodeRef::Get() const {
  CHECK(node_->is_type<TNode>())
      << " type inconsistent, expected " << typeid(TNode).name()
      << " given " << typeid(*this).name();
  return static_cast<const TNode*>(node_.get());
}

inline bool NodeRef::defined() const {
  return node_.get() != nullptr;
}

inline bool NodeRef::operator==(const NodeRef& other) const {
  return node_.get() == other.node_.get();
}

inline bool NodeRef::same_as(const NodeRef& other) const {
  return node_.get() == other.node_.get();
}

inline bool NodeRef::operator<(const NodeRef& other) const {
  return node_.get() < other.node_.get();
}

inline bool NodeRef::operator!=(const NodeRef& other) const {
  return node_.get() != other.node_.get();
}

inline size_t NodeRef::hash() const {
  return std::hash<Node*>()(node_.get());
}

inline void NodeRef::reset(std::shared_ptr<Node> node) {
  node_ = node;
}

}  // namespace tvm

// expose the data structure to HalideIR
namespace Halide {
namespace IR {

using tvm::Node;
using tvm::NodeRef;
using tvm::AttrVisitor;

}  // namespace IR
}  // namespace Halide

namespace std {
template <>
struct hash<::tvm::NodeRef> {
  std::size_t operator()(const ::tvm::NodeRef& k) const {
    return k.hash();
  }
};

}  // namespace std

#endif  // TVM_NODE_H_

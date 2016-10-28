/*!
 *  Copyright (c) 2016 by Contributors
 * \file Range.h
 * \brief The Range data structure
 */
#ifndef HALIDE_IR_RANGE_H_
#define HALIDE_IR_RANGE_H_

#include <memory>
#include "Expr.h"

namespace Halide {
namespace IR {

// Internal node container of Range
class RangeNode;

/*! \brief Node range */
class Range : public NodeRef {
 public:
  /*! \brief constructor */
  Range() {}
  Range(std::shared_ptr<Node> n) : NodeRef(n) {}
  inline Range(Expr min, Expr extent);
  /*!
   * \brief access the internal node container
   * \return the pointer to the internal node container
   */
  inline const RangeNode* operator->() const;
};

/*! \brief range over one dimension */
class RangeNode : public Node {
 public:
  /*! \brief beginning of the node */
  Expr min;
  /*! \brief the extend of range */
  Expr extent;
  /*! \brief constructor */
  RangeNode() {}
  RangeNode(Expr min, Expr extent) : min(min), extent(extent) {}
  const char* type_key() const final {
    return "Range";
  }
  void VisitAttrs(IR::AttrVisitor* v) final {
    v->Visit("min", &min);
    v->Visit("extent", &extent);
  }
};

// implements of inline functions
inline const RangeNode* Range::operator->() const {
  return static_cast<const RangeNode*>(node_.get());
}

inline Range::Range(Expr min, Expr extent)
    : NodeRef(std::make_shared<RangeNode>(min, extent)) {
  internal_assert(min.type() == extent.type())
      << "Region min and extent must have same type\n";
}

// overload print function
inline std::ostream& operator<<(std::ostream &os, const Range& r) {  // NOLINT(*)
  os << "Range(min=" << r->min << ", extent=" << r->extent <<')';
  return os;
}

}  // namespace IR
}  // namespace Halide

#endif  // HALIDE_IR_H_

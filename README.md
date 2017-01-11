# HalideIR

HalideIR is a refactored and enhanced version of IR component of Halide project.
It provides a reusable module to developing new DSL and optimizations for computations.

## Motivation
Halide is a great project and provides a high performing DSL for image processing.
Halide's IR component is an useful module for building new DSLs and adding optimizations.
This repo isolates and refactores Halide's IR for this purpose

Some features highlights include:
- The project modularized
- The IR component is isolated from the front-end spec completely without dependency from LLVM
  - Note: If a codegen compoenent is needed, dependency on LLVM is still needed.
- Making all IR serializable, and publically accessible from front-end language(python).
- Ability to add new IR Expression nodes without modify the original codebase
  - This is important for building new DSL on top of IR.

Many of the features solves pain points raised in original project.
Solve these problems involve re-organization and refactoring with effects spans throughout entire codebase.
Because we have a limited goal(isolate and enhance IR) and are not bounded by compatibility issues,
we can move quickier in making these improvements happen.

## Project Structure

Based on code from Halide(version aa5d5514f179bf0ffe1a2dead0c0eb7300b4069a).
The code is componetized into logical components

- tvm: TVM related base code, basic data structures.
- base: Basic code
- ir: The IR data structure
- pass: pass functions on IR

## Changes to Original Project
- Codestyle: keep old files in old code style
  - Evolve new files to Google C style where linter is available.
- IR is completely isolated from front-end language(function, schedule)
  - This can allow other front-end development based on IR easier.
- Replace implmentation of Float16.cpp with a simpler version that does not depend on LLVM
- Removed introspection code that depends on LLVM
- Folder ir, base now invariant of LLVM

### Changes in IR Data Structure
- IntrusivePtr
  - Remove IntrusivePtr, change everything based on std::shared_ptr
  - See base/Node.h
  - Add support for IR reflection via NodeRef.VisitAttrs
  - All the IR is constructable from python via TVM
    - This enables quick proptyping and debuging from python
  - This also gives us the need
- Call
  - Remove Parameter, BufferPtr from Variable and Call
  - Parameter, BufferPtr could be replaced by higher level wrapping of new Operation(Function) variants
- FunctionContent
  - Make FunctionBaseNode abstract, to replace FunctionContent
- Provide, Realize, ProducerConsumer, Call
  - Remove use of string name matching of function
  - When place where function is needed, use FunctionRef
  - FunctionRef is uniqued matched by internal pointer.
- Variable, Store, Allocate, Free, For, Load, Let, LetStmt
  - Remove use of string name matching of Variable
  - When place where variable is needed, use VarExpr
  - VarExpr is uniqued matched by internal pointer.
- Variable
  - Rename Variable.name -> Variable.name_hint, to avoid confusion.
  - Variable.name_hint is not used to uniquely identify the variable.
  - When possible, encourage SSA form of IR.
- Provide, Realize
  - Change Provide and Realize to associate with value_index.
  - Original Provide/Realize with multiple values can be represented by several Provide and Realize chained together
  - This allows outputs of a Function to have different shapes.
- Make every field the IR reflectable and accessible from python
  - std::vector<> - > Array<>(tvm/container.h)
  - struct Range ->  Range(Range.h)
- Range
  - Remove constructor Range(min, extent) to Range::make_with_min_extent(min, extent)
  - The original constructor could make new user confuse since a typical way is
    range(begin, end) in both c++ and python

### Changes in IR Pass
- Add a more flexible RTTI and dynamic dispatch support, see src/tvm/node.h
  - IRFunctor allows plugin of new IR Node more easily, without chaning interface
- Remove use of string name mapping in the Scope
- Move div_imp, mod_imp from Simplify.h to Divmod.h to avoid cyclic include
- Remove Scope in Substitute to check name conflicts because we use variable pointer matching
- Add Expr&/Stmt& to visit interface in the IRVisitor and IRMutator

## Adapt Pass from Halide
It is possible to reuse passes from Halide project with some adaptation.
Most of adaptations are simple rewriting to respect the new IR data structure and IRVisitor/IRMutator interface.
- IRVisitor, IRMutator:
  - They now takes two arguments, const pointer to specific IR type, and const Expr& (due to use of shared_ptr)
  - We can return the Expr argument if return self is needed.
- Places where matching by name is needed(variable and function name)
  - They are changed to FunctionRef and VarExpr, need to change to match by value
  - As a result most IR are in SSA and nested scope is not needed, except for in the ConvertSSA pass.

# HalideIR

## Project Structure

The code is componetized into logical components

- tvm: TVM related base code, basic data structures.
- base: Basic code
- ir: The IR data structure
- pass: Pass functions on IR

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
  - std::vector<> - > Array<>(tvm/array)
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
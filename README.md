# HalideIR

## Project Structure

The code is componetized into logical components

- tvm: TVM related base code, basic data structures.
- base: Basic code
- ir: The IR data structure
- pass: Pass functions on IR
- codegen: Target related codgen

Goal: base, ir, pass are invariant of LLVM, codegen can depend on LLVM

## Changes to Original Project
- Codestyle: keep old files in old code style
  - Evolve new files to Google C style where linter is available.
- IR is completely isolated from front-end language(function, schedule)
  - This can allow other front-end development based on IR easier.
- Replace implmentation of Float16.cpp with a simpler version that does not depend on LLVM
- Removed introspection code that depends on LLVM
- Folder ir, base now invariant of LLVM
- Remove Parameter, BufferPtr from Variable and Call
  - Parameter, BufferPtr could be replaced by higher level wrapping of structs that contains unbinded Variables.
- Remove IntrusivePtr, change everything based on std::shared_ptr
  - See base/Node.h
  - Add support for IR reflection via NodeRef.VisitAttrs
  - All the IR is constructable from python via TVM
    - This enables quick proptyping and debuging from python
- Make FunctionBaseNode abstract, to replace FunctionContent
- Remove use of string name matching of variable and function
  - When place where variable is needed, use VarExpr
  - When place where function is needed, use FunctionRef
  - VarExpr and FunctionRef are uniqued matched by internal pointer.
  - Rename Variable.name -> Variable.name_hint, to avoid confusion.
- Make every field the IR reflectable and accessible from python
  - std::vector<> - > Array<>(tvm/array)
  - struct Range ->  Range(Range.h)
- Remove use of string name mapping in the Scope
- Move div_imp, mod_imp from Simplify.h to Divmod.h to avoid cyclic include
- Remove constructor Range(min, extent) to Range::make_with_min_extend(min, extent)
  - The original constructor could make new user confuse since a typical way is
    range(begin, end) in both c++ and python

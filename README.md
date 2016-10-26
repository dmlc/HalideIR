# HalideIR

## Project Structure

The code is componetized into logical components

- base: Basic code
- ir: The IR data structure
- pass: Pass functions on IR
- codegen: Target related codgen

Goal: base, ir, pass are invariant of LLVM, codegen can depend on LLVM

## Changes to Original Project
- Replace implmentation of Float16.cpp with a simpler version that does not depend on LLVM
- Removed introspection code that depends on LLVM
- Folder ir, base now invariant of LLVM
- Remove Parameter, BufferPtr from Variable and Function
  - Parameter, BufferPtr could be replaced by higher level wrapping of structs that contains unbinded Variables.
- Remove IntrusivePtr, change everything based on std::shared_ptr
  - See base/Node.h
  - Add support for IR reflection via NodeRef.VisitAttrs
  - TODO(tqchen) make everything serializable

## TODO: tqchen

- Make FunctionContent abstract, allow plugin of user defintion function structure as opposed to Halide's function
  - This also removes dependency of IR from front-end language
- Changes all string matching to unique pointer matching
  - Remove name of Call, Provide
    - name is changed to FunctionPtr
  - Rename name of Variable to var_name, add function name() to Variable.
    - This allows error message in legacy code.
- LetStmt
  - Change name to Variable


## TODO haichen
- Add simplify to pass

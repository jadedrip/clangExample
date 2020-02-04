# clangExample

**CLangToLLVM**

​    Parse C++ header file to llvm IR, convert CLang::Type to llvm::Type

这个例子解析 C++ 的头文件，获取 AST，然后把函数转换为 llvm IR形式的函数。

codeGenTypes.ConvertType 可以用来把 CLang 的 Type 转换为 llvm 的 llvm::Type，也可以用来转换函数。


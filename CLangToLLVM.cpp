#include <cstdio>
#include <string>
#include <sstream>
#include <memory>
#include <iostream>
#include <regex>
#include <llvm/IR/LLVMContext.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Basic/TargetOptions.h>
#include <clang/Basic/TargetInfo.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/CodeGen/ModuleBuilder.h>
#include <clang/Parse/ParseAST.h>
#include <clang/../../lib/CodeGen/CodeGenModule.h>
#include <llvm/Support/Host.h>    
#include <llvm/Support/raw_ostream.h>
#include "llvm/Support/raw_os_ostream.h"

using namespace clang;
using namespace std;

llvm::LLVMContext llvmContext;
CodeGenerator* codeGenerator;

llvm::raw_os_ostream os(std::clog);
// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class FunctionASTVisitor : public RecursiveASTVisitor<FunctionASTVisitor>
{
public:
	bool VisitFunctionDecl(FunctionDecl* f) {
		auto& c = f->getASTContext();
		// Only function definitions (with bodies), not declarations.
		auto returnType = f->getReturnType();
		auto name = f->getName();
		std::vector<llvm::Type*> types;
		auto& cgm = codeGenerator->CGM();
		auto& codeGenTypes = cgm.getTypes();

		f->print(os);
		os << "\r\n";
		// os.flush();

		// Can't convert templated function.
		if (f->isTemplated()) return true;
		if (codeGenTypes.isFuncTypeConvertible(f->getFunctionType())) {
			llvm::Type* ft = codeGenTypes.ConvertType(f->getType());
			os << "    " << name.str() << " : ";
			ft->print(os);
			os << "\r\n";
			assert(ft);
		}
		os.flush();
		return true;
	}
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class CHeaderASTConsumer : public ASTConsumer
{
public:
	// Override the method that gets called for each parsed top-level
	// declaration.
	virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
		for (DeclGroupRef::iterator b = DR.begin(), e = DR.end();
			b != e; ++b)
			// Traverse the declaration using our AST visitor.
			Visitor.TraverseDecl(*b);
		return true;
	}

private:
	FunctionASTVisitor Visitor;
};

// Set options for C++
void setLangOpt(LangOptions& lo) {
	lo.MSVCCompat = 1;
	lo.MicrosoftExt = 1;
	lo.AsmBlocks = 1;
	lo.CPlusPlus = 1;
	lo.CPlusPlus11 = 1;
	lo.ObjCDefaultSynthProperties = 1;
	lo.ObjCInferRelatedResultType = 1;
	lo.LineComment = 1;
	lo.Bool = 1;
	lo.WChar = 1;
	lo.DeclSpecKeyword = 1;
	lo.GNUMode = 0;
	lo.GNUKeywords = 0;
	lo.ImplicitInt = 0;
	lo.Digraphs = 1;
	lo.CXXOperatorNames = 1;
	lo.Exceptions = 1;
	lo.CXXExceptions = 1;
	lo.ThreadsafeStatics = 1;
	lo.ModulesSearchAll = 1;
	lo.NoInlineDefine = 1;
	lo.Deprecated = 1;
	lo.DelayedTemplateParsing = 1;
	lo.MSCompatibilityVersion = 160000000;
}

int main(int argc, char* argv[])
{
	if (argc != 2) {
		llvm::errs() << "Usage: rewritersample <filename>\n";
		return 1;
	}

	// CompilerInstance will hold the instance of the Clang compiler for us,
	// managing the various objects needed to run the compiler.
	CompilerInstance TheCompInst;
	TheCompInst.createDiagnostics();

	// Initialize target info with the default triple for our platform.
	auto TO = std::make_shared<TargetOptions>();
	(*TO).Triple = llvm::sys::getDefaultTargetTriple();
	TargetInfo* TI = TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
	TheCompInst.setTarget(TI);

	TheCompInst.createFileManager();
	FileManager& FileMgr = TheCompInst.getFileManager();
	TheCompInst.createSourceManager(FileMgr);
	SourceManager& SourceMgr = TheCompInst.getSourceManager();
	setLangOpt(TheCompInst.getLangOpts());

	std::string includePath = std::getenv("INCLUDE_PATH");
	if (!includePath.empty()) {
		auto& opts = TheCompInst.getHeaderSearchOpts();
		std::regex re{ ";" };
		for (auto i = sregex_token_iterator(includePath.begin(), includePath.end(), re, -1);
			i != sregex_token_iterator(); i++) {
			if (i->first != i->second)
				opts.AddPath(i->str(), frontend::System, false, false);
		};
	}
	// TheCompInst.createPreprocessor();
	///TU_Complete   TU_Prefix  TU_Module
	TheCompInst.createPreprocessor(TU_Complete);
	TheCompInst.createASTContext();

	// Set the main file handled by the source manager to the input file.
	const FileEntry* FileIn = FileMgr.getFile(argv[1]);
	SourceMgr.setMainFileID(SourceMgr.createFileID(FileIn, SourceLocation(), SrcMgr::C_User));

	TheCompInst.getDiagnosticClient().BeginSourceFile(
		TheCompInst.getLangOpts(),
		&TheCompInst.getPreprocessor());

	// Create an AST consumer instance which is going to get called by
	// ParseAST.
	CHeaderASTConsumer TheConsumer;

	codeGenerator = clang::CreateLLVMCodeGen(TheCompInst.getDiagnostics(), "hpar", TheCompInst.getHeaderSearchOpts(),
		TheCompInst.getPreprocessorOpts(),
		TheCompInst.getCodeGenOpts(), llvmContext);
	codeGenerator->Initialize(TheCompInst.getASTContext());
	// Parse the file to AST, registering our consumer as the AST consumer.
	ParseAST(TheCompInst.getPreprocessor(), &TheConsumer,
		TheCompInst.getASTContext());
	return 0;
}


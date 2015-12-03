# Access the abstract syntax tree created by the swift-compiler 

Apple open sourced the swift-compiler (and some related projects, like the swift-standard-library) a few days ago. Thus, it became the second compiler in the last years that was open sourced by Apple - the first one was clang. Releasing clang as open source has been a great success - it got widely adopted across all systems and has a great community. One of the key features - at least in my eyes - is its modularity. Chris Lattner designed clang from the beginning on to be reusable in other projects, like IDEs. And as swift is also created by Chris Lattner, the same applies to the swift-compiler.

## The coding-part
The coding-part only needs a few steps to trigger a compilation and access the AST.

- initialize LLVM
- create a CompilerInstance
- create a CompilerInvocation
- setup the CompilerInstance by using the CompilerInvocation
- call _performSema_ on the CompilerInstance

The abstract syntax tree is now created. You may access it by implementing an ASTWalker. A complete example: 

	#include <iostream>
	
	#include <swift/Basic/SourceManager.h>
	#include <swift/AST/DiagnosticEngine.h>
	#include <swift/Frontend/Frontend.h>
	#include <swift/Driver/FrontendUtil.h>
	#include <swift/Basic/LLVMInitialize.h>
	
	#include <llvm/ADT/ArrayRef.h>
	
	using namespace std;
	using namespace swift;
	using namespace swift::driver;
	
	void anchorForGetMainExecutable() {}
	
	// Get informed about errors that occur while the compilation
	class BasicDiagnosticConsumer : public DiagnosticConsumer {
	public:
	  void handleDiagnostic(SourceManager &SM, SourceLoc Loc,
	                        DiagnosticKind Kind, StringRef Text,
	                        const DiagnosticInfo &Info) override {
	    cerr << "diagnostic: " << Text.str() << endl;
	  }
	};
	
	// Subclass ASTWalker to traverse all declarations, expressions, patterns, types, ...
	class BasicASTWalker : public ASTWalker {
	   bool walkToDeclPre(Decl *D) override {
	    cout << "Declaration " << D->getKindName(D->getKind()).str() << endl;
	    return true;
	  }
	   pair<bool, Expr *> walkToExprPre(Expr *E) override {
	    cout << "Expression " << E->getKindName(E->getKind()).str() << endl;
	    return { true, E };
	  }
	};
	
	int main(int argc, const char * argv[]) {
	  INITIALIZE_LLVM(argc, argv);
	  
	  // setup a compiler-instance for a specific file
	  CompilerInstance instance;
	  unique_ptr<CompilerInvocation> invocation = createCompilerInvocation(llvm::makeArrayRef(argv + 1, argv + argc), instance.getDiags());
	  
	  BasicDiagnosticConsumer diagnosticConsumer;
	  instance.getDiags().addConsumer(diagnosticConsumer);
	  
	  // this might be helpful for debugging in case that the resource-/sdk-paths are misconfigured
	  invocation->getClangImporterOptions().DumpClangDiagnostics = true;
	  
	  const bool didInstanceSetupFail = instance.setup(*invocation);
	  cout << "setting up compilerinstance: " << (didInstanceSetupFail ? "failed" : "succeedd") << endl;
	  
	  // create the AST
	  instance.performSema();
	  
	  // now, your tool can do whatever you want with the AST
	  BasicASTWalker astWalker;
	  instance.getMainModule()->walk(astWalker);
	  
	  return 0;
	}	

## Compile the program
We now have to compile the code above. For this, it is necessary to check out and compile swift as described at https://github.com/apple/swift, so that the headers and libraries we need are available. It would be sufficient to link only some of the libraries to get the code running. But i prefer to link all of them from the beginning on, instead of incrementally adding them whenever you need it - it's a cumbersome process of doing so. Beside the header-search-paths and library-linkings, we have to set some macros and disable RTTI. All of this can be done by a xcconfig that is reusable.

	// Set this to the path where you checked out all swift-related projects. 
	// All other variables should fit without further adjustments
	BASE_DIR=$HOME/develop/thirdparty/swift
	
	COMPILATION_DIR=$BASE_DIR/build/Ninja-DebugAssert
	
	OTHER_LDFLAGS=-L$COMPILATION_DIR/swift-macosx-x86_64/lib -L$COMPILATION_DIR/llvm-macosx-x86_64/lib -L$COMPILATION_DIR/cmark-macosx-x86_64/src -L$COMPILATION_DIR/swift-macosx-x86_64/lib/swift/macosx $COMPILATION_DIR/cmark-macosx-x86_64/src/libcmark.a -lclangAnalysis -lclangAPINotes -lclangARCMigrate -lclangAST -lclangASTMatchers -lclangBasic -lclangCodeGen -lclangDriver -lclangDynamicASTMatchers -lclangEdit -lclangFormat -lclangFrontend -lclangFrontendTool -lclangIndex -lclangLex -lclangParse -lclangRewrite -lclangRewriteFrontend -lclangSema -lclangSerialization -lclangStaticAnalyzerCheckers -lclangStaticAnalyzerCore -lclangStaticAnalyzerFrontend -lclangTooling -lclangToolingCore -lLLVMAArch64AsmParser -lLLVMAArch64AsmPrinter -lLLVMAArch64CodeGen -lLLVMAArch64Desc -lLLVMAArch64Disassembler -lLLVMAArch64Info -lLLVMAArch64Utils -lLLVMAnalysis -lLLVMARMAsmParser -lLLVMARMAsmPrinter -lLLVMARMCodeGen -lLLVMARMDesc -lLLVMARMDisassembler -lLLVMARMInfo -lLLVMAsmParser -lLLVMAsmPrinter -lLLVMBitReader -lLLVMBitWriter -lLLVMCodeGen -lLLVMCore -lLLVMDebugInfoDWARF -lLLVMDebugInfoPDB -lLLVMExecutionEngine -lLLVMInstCombine -lLLVMInstrumentation -lLLVMInterpreter -lLLVMipo -lLLVMIRReader -lLLVMLibDriver -lLLVMLineEditor -lLLVMLinker -lLLVMLTO -lLLVMMC -lLLVMMCDisassembler -lLLVMMCJIT -lLLVMMCParser -lLLVMMIRParser -lLLVMObjCARCOpts -lLLVMObject -lLLVMOption -lLLVMOrcJIT -lLLVMPasses -lLLVMProfileData -lLLVMRuntimeDyld -lLLVMScalarOpts -lLLVMSelectionDAG -lLLVMSupport -lLLVMSymbolize -lLLVMTableGen -lLLVMTarget -lLLVMX86AsmParser -lLLVMX86AsmPrinter -lLLVMX86CodeGen -lLLVMX86Desc -lLLVMX86Disassembler -lLLVMX86Info -lLLVMX86Utils  -lLLVMTransformUtils -lLLVMVectorize -lSourceKitCore -lsourcekitdAPI -lSourceKitSupport -lSourceKitSwiftLang -lswiftAST -lswiftASTSectionImporter -lswiftBasic -lswiftClangImporter -lswiftDriver -lswiftFrontend -lswiftIDE -lswiftImmediate -lswiftIRGen -lswiftLLVMPasses -lswiftMarkup -lswiftOption -lswiftParse -lswiftPrintAsObjC -lswiftSema -lswiftSerialization -lswiftSIL -lswiftSILAnalysis -lswiftSILGen -lswiftSILPasses -lswiftSILPassesUtils -lswiftRuntime -lncurses -lz
	HEADER_SEARCH_PATHS=$BASE_DIR/swift/include $COMPILATION_DIR/llvm-macosx-x86_64/include $BASE_DIR/clang/include $BASE_DIR/llvm/include $COMPILATION_DIR/swift-macosx-x86_64/include
	OTHER_CPLUSPLUSFLAGS=-Wno-shorten-64-to-32 -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -fno-rtti

## Perform the program on a swift-file
To see the output of the _BasicASTWalker_ for a specific swift-file, you have to perform it in a terminal and pass

- the path of the swift-file
- the path to the resource-directory of your compiled swift-instance, for example _~/develop/thirdparty/swift/build/Ninja-DebugAssert/swift-macosx-x86\_64/lib/swift_

The output of the AST for a simple ```print("Hello, World!")```-program should look like

	Declaration TopLevelCode
	Expression Call
	Expression DeclRef
	Expression TupleShuffle
	Expression Paren
	Expression Erasure
	Expression Call
	Expression ConstructorRefCall
	Expression Type
	Expression DeclRef
	Expression StringLiteral

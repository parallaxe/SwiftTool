//
//  main.cpp
//  SwiftTool
//
//  Created by Hendrik von Prince on 04/12/15.
//  Copyright © 2015 Hendrik von Prince. All rights reserved.
//

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

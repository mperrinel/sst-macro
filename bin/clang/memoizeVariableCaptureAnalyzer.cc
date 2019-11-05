/**
Copyright 2009-2019 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2019, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#include "memoizeVariableCaptureAnalyzer.h"
#include "clang/Analysis/Analyses/ExprMutationAnalyzer.h"

#include "clangGlobals.h"
#include "util.h"

#include <optional>

namespace {
using namespace clang;
using namespace ast_matchers;

void forStmtVariableAutoMatcher(clang::ForStmt const *FS,
                                std::string const &namedDeclsToCapture) {
  // clang-format off
  /*
   *  Get all the Variables declared in our ForStmt that we care about
   */
  auto getLoopVarsOrAnything = anyOf(
         forEachDescendant(
           declStmt(forEach(varDecl().bind("Declared")))
         ),
         anything()
       );

  auto dependsOnLoopDeclaredVar = findAll(
          declRefExpr(to(varDecl(equalsBoundNode("Declared"))))
       );

  auto getConditionExpr = hasCondition(expr().bind("ConditionExpr"));

  auto getAllConditions = findAll(
      stmt(anyOf(forStmt(getConditionExpr), ifStmt(getConditionExpr),
                 whileStmt(getConditionExpr), doStmt(getConditionExpr))));

  auto hasMatchingAncestorExpr = [](std::string const &str) {
    return hasAncestor(expr(equalsBoundNode(str)));
  };

  auto getNestedConditionExprs = forEachDescendant(
         expr(
           anyOf(
             equalsBoundNode("ConditionExpr"),
             hasMatchingAncestorExpr("ConditionExpr")
           ),
           unless(dependsOnLoopDeclaredVar)
         ).bind("InnerExpr")
       );

  auto filterForTopMatch = forEachDescendant(
         expr(
           equalsBoundNode("InnerExpr"),
           unless(hasMatchingAncestorExpr("InnerExpr"))
         ).bind("FinalExpr")
       );

  auto BN = match(
      stmt(
        getLoopVarsOrAnything,
        getAllConditions,
        getNestedConditionExprs,
        filterForTopMatch
      ), *FS, *sst::activeASTContext); // FS is a ForStmt
  // clang-format on

  llvm::errs() << "\n";
  // FS->dumpPretty(*sst::activeASTContext);
  FS->dumpColor();
  llvm::errs() << "\nDeclared\n";
  for (auto const &VD : ::detail::toPtrSet<VarDecl>(BN, "Declared")) {
    llvm::errs() << VD->getNameAsString() << ", ";
  }

  llvm::errs() << "\n\nConditionsExpr\n";
  for (auto const &exp : ::detail::toPtrSet<Expr>(BN, "ConditionExpr")) {
    exp->dumpPretty(*sst::activeASTContext);
    llvm::errs() << "\n";
  }

  llvm::errs() << "\nFinal Exprs\n";
  for (auto const &exp : ::detail::toPtrSet<Expr>(BN, "FinalExpr")) {
    exp->dumpPretty(*sst::activeASTContext);
    llvm::errs() << "\n";
  }

  llvm::errs() << "\nFiltered Exprs\n";
  llvm::SmallPtrSet<Expr const *, 4> filtered;
  for (auto const &exp : ::detail::toPtrSet<Expr>(BN, "FinalExpr")) {
    bool found = false;

    if (!exp->getType()->isFundamentalType()) {
      llvm::errs() << "Type: " << exp->getType().getAsString() << "\n";
      break;
    }

    for (auto ptr : filtered) {
      PrettyPrinter printer;
      printer.print(exp);
      auto expstr = printer.str();

      PrettyPrinter printer2;
      printer2.print(ptr);
      auto ptrstr = printer2.str();

      if (expstr == ptrstr) {
        found = true;
        break;
      }
    }

    if (!found) {
      filtered.insert(exp);
    }
  }

  for (auto const &exp : filtered) {
    exp->dumpPretty(*sst::activeASTContext);
    llvm::errs() << "\n";
  }

  llvm::errs() << "\n\n\n";
}

} // namespace

void memoizationAutoMatcher(clang::Stmt const *S,
                            std::string const &namedDecls) {
  if (auto FS = llvm::dyn_cast<clang::ForStmt>(S)) {
    forStmtVariableAutoMatcher(FS, namedDecls);
  } else {
    S->dumpColor();
  }
}

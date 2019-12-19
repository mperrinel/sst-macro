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

#include "astMatchers.h"
#include "clangGlobals.h"
#include "util.h"

namespace {

using namespace clang;
using namespace ast_matchers;

bool exprToLiteral(Expr const *expr) {
  auto noCastExpr = expr->IgnoreCasts();

  switch (noCastExpr->getStmtClass()) {
  case (Stmt::IntegerLiteralClass):
  case (Stmt::FloatingLiteralClass):
  case (Stmt::CharacterLiteralClass):
  case (Stmt::CXXBoolLiteralExprClass):
  case (Stmt::ImaginaryLiteralClass):
  case (Stmt::StringLiteralClass):
    return true;
  default:
    return false;
  }
}

bool isArithmeticType(clang::Expr const *expr) {
  if (expr->getType()->isArithmeticType()) {
    return true;
  }

  return false;
}

auto filterExprs(llvm::SmallPtrSet<Expr const *, 4> const &C) {
  auto getSpelling = [](Expr const *e) {
    std::string str;
    clang::PrintingPolicy Policy(*sst::activeLangOpts);
    llvm::raw_string_ostream os(str);
    e->printPretty(os, nullptr, Policy);
    return str;
  };

  llvm::SmallPtrSet<Expr const *, 4> out;
  llvm::SmallSet<std::string, 4> strings;

  for (auto expr : C) {
    if (!isArithmeticType(expr) || exprToLiteral(expr) ||
        llvm::isa<CallExpr>(expr)) {
      continue;
    }

    auto expr_str = getSpelling(expr);
    if (!strings.count(expr_str)) {
      out.insert(expr);
      strings.insert(expr_str);
    }
  }

  return out;
}

auto bindNestedDeclaredVarDecls(std::string const &str) {
  return forEachDescendant(declStmt(forEach(varDecl().bind(str))));
};

auto bindConditionExpr(std::string const &str) {
  return hasCondition(expr().bind(str));
};

auto hasBoundAncestorExpr(std::string const &str) {
  return hasAncestor(expr(equalsBoundNode(str)));
};

// clang-format off
auto bindAllConditionExprs = [](std::string const& str){
  return findAll(stmt(anyOf(
       forStmt(bindConditionExpr(str)), 
       ifStmt(bindConditionExpr(str)),
       whileStmt(bindConditionExpr(str)), 
       switchStmt(bindConditionExpr(str)), 
       conditionalOperator(bindConditionExpr(str)), 
       binaryConditionalOperator(bindConditionExpr(str)), 
       doStmt(bindConditionExpr(str))
     )));
};
// clang-format on

auto forStmtConditionCaptures(clang::Stmt const *FS) {

  // clang-format off
  auto getLoopVarsOrAnything = anyOf(
         bindNestedDeclaredVarDecls("Declared"),
         anything()
       );

  auto dependsOnLoopDeclaredVar = 
    findAll(declRefExpr(to(varDecl(equalsBoundNode("Declared")))));

  auto getMatchingExprs = forEachDescendant(
         expr(
           unless(dependsOnLoopDeclaredVar),
           anyOf(
             hasBoundAncestorExpr("ConditionExpr"),
             equalsBoundNode("ConditionExpr")
           )
         ).bind("InnerExpr")
       );

  auto rejectListExprs = hasAncestor(arraySubscriptExpr());

  auto filterForTopMatch = forEachDescendant(
         expr( 
           anyOf( // Preserves all inner expressions for anaylsis later
             expr(
               unless(hasBoundAncestorExpr("InnerExpr")),
               unless(rejectListExprs),
               equalsBoundNode("InnerExpr")
             ).bind("FinalExpr"),
             expr(
               equalsBoundNode("InnerExpr")
             ),
             expr(
               callExpr(),
               unless(dependsOnLoopDeclaredVar)
             ).bind("FinalExpr")
           )
         )
       );

  auto BN = match(
      stmt(
        forStmt(bindConditionExpr("ConditionExpr")), 
        getLoopVarsOrAnything,
        getMatchingExprs,
        filterForTopMatch
      ), *FS, *sst::activeASTContext);
  // clang-format on

  return matchers::toPtrSet<Expr>(BN, "FinalExpr");
}

auto stmtConditionCaptures(clang::Stmt const *FS) {

  // clang-format off
  auto getLoopVarsOrAnything = anyOf(
         bindNestedDeclaredVarDecls("Declared"),
         anything()
       );

  auto dependsOnLoopDeclaredVar = 
    findAll(declRefExpr(to(varDecl(equalsBoundNode("Declared")))));

  auto getMatchingExprs = forEachDescendant(
         expr(
           unless(dependsOnLoopDeclaredVar),
           anyOf(
             hasBoundAncestorExpr("ConditionExpr"),
             equalsBoundNode("ConditionExpr")
           )
         ).bind("InnerExpr")
       );

  auto rejectListExprs = hasAncestor(arraySubscriptExpr());

  auto filterForTopMatch = forEachDescendant(
         expr( 
           anyOf( // Preserves all inner expressions for anaylsis later
             expr(
               unless(hasBoundAncestorExpr("InnerExpr")),
               unless(rejectListExprs),
               equalsBoundNode("InnerExpr")
             ).bind("FinalExpr"),
             expr(
               equalsBoundNode("InnerExpr")
             ),
             expr(
               callExpr(),
               unless(dependsOnLoopDeclaredVar)
             ).bind("FinalExpr")
           )
         )
       );

  auto BN = match(
      stmt(
        bindAllConditionExprs("ConditionExpr"),
        getLoopVarsOrAnything,
        getMatchingExprs,
        filterForTopMatch
      ), *FS, *sst::activeASTContext);
  // clang-format on

  return matchers::toPtrSet<Expr>(BN, "FinalExpr");
}

auto getDeclRefExprsToVariables(clang::Stmt const *S,
                                std::string const &namedDeclsRegex) {
  auto matches =
      match(findAll(declRefExpr(to(varDecl(matchesName(namedDeclsRegex))))
                        .bind("DeclRefs")),
            *S, *sst::activeASTContext);

  return matchers::toPtrSet<Expr>(matches, "DeclRefs");
}

auto getLoopVarDeclsInitializers(clang::Stmt const *S) {
  // clang-format off
  auto matches =
      match(
        stmt(
          bindNestedDeclaredVarDecls("Declared"),
          bindAllConditionExprs("ConditionExpr"),
          forEachDescendant(
            declRefExpr(
              hasBoundAncestorExpr("ConditionExpr"),
              to(
                varDecl(
                  hasInitializer(
                    expr(
                      unless(
                        hasDescendant(
                          declRefExpr(to(varDecl(equalsBoundNode("Declared"))))
                        )
                      )
                    ).bind("ExternalInitializer")
                  ),
                  equalsBoundNode("Declared")
                )
              )
            )
          )
        ), *S, *sst::activeASTContext);
  // clang-format on

  return matchers::toPtrSet<Expr>(matches, "ExternalInitializer");
}

} // namespace


llvm::SmallPtrSet<Expr const *, 4>
memoizationAutoMatcher(clang::Stmt const *S, std::string const &namedDeclsRegex,
                       capture::AutoCapture ac) {

  llvm::SmallPtrSet<Expr const *, 4> results;

  if (!namedDeclsRegex.empty()) {
    auto tmp = getDeclRefExprsToVariables(S, namedDeclsRegex);
    results.insert(tmp.begin(), tmp.end());
  }

  using namespace capture;
  if (ac == AutoCapture::AllConditions) {
    auto tmp = stmtConditionCaptures(S);
    results.insert(tmp.begin(), tmp.end());

    tmp = getLoopVarDeclsInitializers(S);
    results.insert(tmp.begin(), tmp.end());
  }

  if (ac == AutoCapture::ForLoopConditions) {
    auto tmp = forStmtConditionCaptures(S);
    results.insert(tmp.begin(), tmp.end());
  }

  return filterExprs(results);
}

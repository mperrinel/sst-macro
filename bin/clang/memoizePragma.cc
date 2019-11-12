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

#include "memoizePragma.h"
#include "astMatchers.h"
#include "clangGlobals.h"
#include "memoizeVariableCaptureAnalyzer.h"
#include "util.h"

namespace {

std::string cleanPath(std::string const &p) {
  // Don't care if it doesn't work on Windows
  auto root_end = p.find_last_of('/') + 1;
  auto out = p.substr(root_end);
  for (auto &c : out) {
    if (c == '.') {
      c = '_';
    }
  }

  return out;
}

std::string generateUniqueFunctionName(clang::SourceLocation const &Loc,
                                       clang::NamedDecl const *Decl,
                                       std::string Prefix) {
  static int counter = 0;
  Prefix += (Prefix.empty() ? "f" : "") + std::to_string(counter++);
  Prefix += "_" + Decl->getNameAsString() + "_";

  auto &SM = *sst::activeSourceManger;
  std::string path = SM.getFilename(Loc).str();
  Prefix += cleanPath(path) + std::to_string(SM.getPresumedLineNumber(Loc));

  return Prefix;
}

auto parseKeyword(PragmaArgMap const &Strings, std::string const &Key) {
  using ContainerType = std::vector<std::string>;

  // If Key was in the arguments and it is not an empty list return it.
  if (auto Vars = Strings.find(Key);
      Vars != Strings.end() && not Vars->second.empty()) {
    return std::make_optional<ContainerType>(Vars->second.begin(),
                                             Vars->second.end());
  }

  return std::optional<ContainerType>();
}

template <typename StmtDecl, typename Container>
auto getAllExprs(StmtDecl const *SD,
                 std::optional<Container> const &VariableNames,
                 bool AutoCapture) {

  std::string NameRegex =
      (VariableNames) ? ::matchers::makeNameRegex(*VariableNames) : "";

  return memoizationAutoMatcher(SD, NameRegex, AutoCapture);
}

template <typename Fn>
std::string
parseExprString(std::vector<memoize::ExpressionStrings> const &Exprs, Fn &&f) {
  std::string out;

  bool isFirst = true;
  for (auto const &Expr : Exprs) {
    std::string comma = (isFirst) ? "" : ", ";
    isFirst = false;

    out += comma + f(Expr);
  }

  return out;
}

std::string declare_start_function(
    std::string const &FuncName,
    std::vector<memoize::ExpressionStrings> const &ExprStrs) {

  std::string out;
  llvm::raw_string_ostream os(out);
  os << "void " << FuncName << "("
     << parseExprString(ExprStrs,
                        [](memoize::ExpressionStrings const &es) {
                          return es.getSrcFileType();
                        })
     << ");";
  return out;
}

std::string
declare_end_function(std::string const &FuncName,
                     std::vector<memoize::ExpressionStrings> const &) {

  std::string out;
  llvm::raw_string_ostream os(out);

  os << "void " << FuncName << "_end();";
  return out;
}

std::string
start_call_site(std::string const &FuncName,
                std::vector<memoize::ExpressionStrings> const &ExprStrs) {
  std::string out;
  llvm::raw_string_ostream os(out);
  os << FuncName << "("
     << parseExprString(ExprStrs,
                        [](memoize::ExpressionStrings const &es) {
                          return es.getExprSpelling();
                        })
     << ");";
  return out;
};

std::string
start_definition(std::string const &FuncName,
                 std::vector<memoize::ExpressionStrings> const &ExprStrs) {

  std::string out;
  llvm::raw_string_ostream os(out);

  std::string argList =
      parseExprString(ExprStrs, [](memoize::ExpressionStrings const &es) {
        return es.getCppType();
      });

  auto captureFlagName = FuncName + "_capture_flag";
  auto captureVarName = FuncName + "_capture_var";

  os << "void " << FuncName << "("
     << parseExprString(
            ExprStrs,
            [var = 0](memoize::ExpressionStrings const &es) mutable {
              auto v = es.getCppType() + " v" + std::to_string(var++);
              return v;
            })
     << "){\n\tstd::call_once(" << captureFlagName << ",\n\t\t[]{"
     << captureVarName << " = "
     << "memoize::getCaptureType<" << argList << ">(\"" << FuncName
     << "\");});\n"
     << "\t" << captureVarName << "->capture_start("
     << "\"" << FuncName << "\", "
     << parseExprString(
            ExprStrs,
            [var = 0](memoize::ExpressionStrings const &es) mutable {
              auto v = "v" + std::to_string(var++);
              return v;
            })
     << ");\n}";

  return out;
}

std::string end_definition(std::string const &FuncName,
                           std::vector<memoize::ExpressionStrings> const &) {

  std::string out;
  llvm::raw_string_ostream os(out);

  auto captureVarName = FuncName + "_capture_var";

  os << "void " << FuncName << "_end("
     << "){\n\t" << captureVarName << "->capture_stop("
     << "\"" << FuncName << "\");\n}";

  return out;
}

std::string end_call_site(std::string const &FuncName,
                          std::vector<memoize::ExpressionStrings> const &) {

  std::string out;
  llvm::raw_string_ostream os(out);

  os << FuncName << "_end();";
  return out;
}

std::string write_static_capture_variable(
    std::string const &FuncName,
    std::vector<memoize::ExpressionStrings> const &ExprStrs) {

  std::string argList =
      parseExprString(ExprStrs, [](memoize::ExpressionStrings const &es) {
        return es.getCppType();
      });

  std::string out;
  llvm::raw_string_ostream os(out);

  os << "std::once_flag " << FuncName + "_capture_flag;\n";
  os << "static memoize::Capture<" << argList << "> * "
     << FuncName + "_capture_var = nullptr;";

  return out;
}

} // namespace

namespace memoize {
SSTMemoizePragma::SSTMemoizePragma(clang::SourceLocation Loc,
                                   clang::CompilerInstance &CI,
                                   PragmaArgMap &&PragmaStrings)
    : VariableNames_(parseKeyword(PragmaStrings, "variables")),
      ExtraExpressions_(parseKeyword(PragmaStrings, "extra_exressions")) {
  if (VariableNames_) {
    auto &strs = *VariableNames_;
    if (auto ac = std::find(strs.begin(), strs.end(), "auto");
        ac == strs.end()) {
      DoAutoCapture = false;
    } else {
      strs.erase(ac); // Don't actually look for a variable named "auto"
    }
  }
}

ExpressionStrings::ExpressionStrings(clang::Expr const *e)
    : spelling(getStmtSpelling(e)) {
  if (sst::activeLangOpts->CPlusPlus) {
    cppType = getExprDesugaredTypeSpelling(e);
  } else {
    cType = getExprDesugaredTypeSpelling(e);

    auto cppLO = clang::LangOptions();
    cppLO.CPlusPlus = true;
    cppType = getExprDesugaredTypeSpelling(e, &cppLO);
  }
}
std::string ExpressionStrings::getExpressionLabel() const {
  return "\"" + cType.value_or(cppType) + " " + spelling + "\"";
}
std::string ExpressionStrings::getSrcFileType() const {
  return cType.value_or(cppType);
}
std::string const &ExpressionStrings::getCppType() const { return cppType; }
std::string const &ExpressionStrings::getExprSpelling() const {
  return spelling;
}

void SSTMemoizePragma::activate(clang::Stmt *S, clang::Rewriter &R,
                                PragmaConfig &Cfg) {

  S->dumpPretty(*sst::activeASTContext);
  for (auto Expr : getAllExprs(S, VariableNames_, DoAutoCapture)) {
    ExprStrs_.emplace_back(Expr);
  }

  auto ParentDecl = getNonNull(matchers::getParentDecl(S));
  auto FuncName = generateUniqueFunctionName(getStart(S), ParentDecl, "");

  static_capture_decl_ = write_static_capture_variable(FuncName, ExprStrs_);
  start_capture_definition_ = start_definition(FuncName, ExprStrs_);;
  stop_capture_definition_ = end_definition(FuncName, ExprStrs_);

  R.InsertTextBefore(getStart(ParentDecl),
                     declare_start_function(FuncName, ExprStrs_) + "\n");
  R.InsertTextBefore(getStart(ParentDecl),
                     declare_end_function(FuncName, ExprStrs_) + "\n");

  R.InsertTextBefore(getStart(S), start_call_site(FuncName, ExprStrs_) + "\n");
  R.InsertTextAfterToken(getEnd(S), end_call_site(FuncName, ExprStrs_) + "\n");
}

void SSTMemoizePragma::activate(clang::Decl *D, clang::Rewriter &R,
                                PragmaConfig &Cfg) {}

void SSTMemoizePragma::deactivate(PragmaConfig &cfg) {
  auto &vec = cfg.globalCppFunctionsToWrite;
  if (std::none_of(vec.begin(), vec.end(), [](auto const &pragma_string) {
        return pragma_string.second == "#include \"capture.h\"\n";
      })) {
    vec.push_back(std::make_pair(this, "#include \"capture.h\"\n"));
  }

  vec.push_back(std::make_pair(this, static_capture_decl_ + "\n"));
  vec.push_back(std::make_pair(this, start_capture_definition_ + "\n"));
  vec.push_back(std::make_pair(this, stop_capture_definition_ + "\n"));
}
} // namespace memoize

static PragmaRegister<SSTArgMapPragmaShim, memoize::SSTMemoizePragma, true>
    memoizePragma("sst", "memoize", pragmas::MEMOIZE);

static PragmaRegister<SSTArgMapPragmaShim, memoize::SSTMemoizePragma, false>
    ompMemoizePragma("omp", "parallel", pragmas::MEMOIZE);

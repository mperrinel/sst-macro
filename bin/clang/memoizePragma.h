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

#ifndef bin_clang_memoizePragma_h
#define bin_clang_memoizePragma_h

#include "memoizeVariableCaptureAnalyzer.h"
#include "pragmas.h"

#include <optional>
#include <vector>

namespace memoize {

class ExpressionStrings {
public:
  ExpressionStrings(clang::Expr const *e);

  /* Returns the type plus expression in quotes so true would return
   * either "_Bool true" or "bool true" depending on the language
   */
  std::string getExpressionLabel() const;
  std::string getSrcFileType() const;
  std::string const &getCppType() const;
  std::string const &getExprSpelling() const;

private:
  std::string cppType; // Must always have this for the extern cpp file
  std::optional<std::string> cType; // Only needed if the active lang is C
  std::string spelling;             // The actual expression
};

/*
 * Will automatically determine which expressions to capture unless the user
 * provides variables.  If the user provides variables and wants that in
 * addition to automatic capture they should include the word auto in the
 * list of things to capture.
 *
 */
class SSTMemoizePragma : public SSTPragma {
public:
  SSTMemoizePragma(clang::SourceLocation Loc, PragmaArgMap &&PragmaStrings);

  void activate(clang::Stmt *S) override;
  void activate(clang::Decl *D) override;
  void deactivate() override;

private:
  std::optional<std::vector<std::string>> VariableNames_;
  std::optional<std::vector<std::string>> ExtraExpressions_;

  capture::AutoCapture DoAutoCapture_;
  std::vector<ExpressionStrings> ExprStrs_;

  std::string static_capture_decl_;
  std::string start_capture_definition_;
  std::string stop_capture_definition_;

  virtual bool isOMP() { return false; }
};

class SSTMemoizeOMPPragma : public SSTMemoizePragma {
public:
  SSTMemoizeOMPPragma(clang::SourceLocation Loc, PragmaArgMap &&PragmaStrings)
      : SSTMemoizePragma(Loc, std::move(PragmaStrings)) {}

private:
  bool isOMP() override { return true; }
};

} // namespace memoize

#endif // bin_clang_memoizePragma_h

/*
 * Copyright (c) 2015 PLUMgrid, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Rewrite/Core/Rewriter.h>

#include "table_desc.h"

namespace clang {
class ASTConsumer;
class ASTContext;
class CompilerInstance;
}

namespace llvm {
class raw_ostream;
class StringRef;
}

namespace ebpf {

// Helper visitor for constructing a string representation of a key/leaf decl
class BMapDeclVisitor : public clang::RecursiveASTVisitor<BMapDeclVisitor> {
 public:
  explicit BMapDeclVisitor(clang::ASTContext &C, std::string &result);
  bool TraverseRecordDecl(clang::RecordDecl *Decl);
  bool VisitRecordDecl(clang::RecordDecl *Decl);
  bool VisitFieldDecl(clang::FieldDecl *Decl);
  bool VisitBuiltinType(const clang::BuiltinType *T);
  bool VisitTypedefType(const clang::TypedefType *T);
  bool VisitTagType(const clang::TagType *T);
 private:
  clang::ASTContext &C;
  std::string &result_;
};

// Helper visitor for constructing a fscanf routine for key/leaf decl
class BScanfVisitor : public clang::RecursiveASTVisitor<BScanfVisitor> {
 public:
  explicit BScanfVisitor(clang::ASTContext &C);
  bool TraverseRecordDecl(clang::RecordDecl *Decl);
  bool VisitRecordDecl(clang::RecordDecl *Decl);
  bool VisitFieldDecl(clang::FieldDecl *Decl);
  bool VisitBuiltinType(const clang::BuiltinType *T);
  bool VisitTypedefType(const clang::TypedefType *T);
  bool VisitTagType(const clang::TagType *T);
  void finalize(std::string &result);
 private:
  clang::ASTContext &C;
  size_t n_args_;
  std::string fmt_;
  std::string args_;
  std::string type_;
};

// Type visitor and rewriter for B programs.
// It will look for B-specific features and rewrite them into a valid
// C program. As part of the processing, open the necessary BPF tables
// and store the open handles in a map of table-to-fd's.
class BTypeVisitor : public clang::RecursiveASTVisitor<BTypeVisitor> {
 public:
  explicit BTypeVisitor(clang::ASTContext &C, clang::Rewriter &rewriter,
                        std::map<std::string, TableDesc> &tables);
  bool TraverseCallExpr(clang::CallExpr *Call);
  bool TraverseMemberExpr(clang::MemberExpr *E);
  bool VisitFunctionDecl(clang::FunctionDecl *D);
  bool VisitCallExpr(clang::CallExpr *Call);
  bool VisitVarDecl(clang::VarDecl *Decl);
  bool VisitMemberExpr(clang::MemberExpr *E);
  bool VisitDeclRefExpr(clang::DeclRefExpr *E);
  bool VisitBinaryOperator(clang::BinaryOperator *E);
  bool VisitImplicitCastExpr(clang::ImplicitCastExpr *E);

 private:
  clang::ASTContext &C;
  clang::Rewriter &rewriter_;  /// modifications to the source go into this class
  llvm::raw_ostream &out_;  /// for debugging
  std::map<std::string, TableDesc> &tables_;  /// store the open FDs
  std::vector<clang::ParmVarDecl *> fn_args_;
};

// A helper class to the frontend action, walks the decls
class BTypeConsumer : public clang::ASTConsumer {
 public:
  explicit BTypeConsumer(clang::ASTContext &C, clang::Rewriter &rewriter,
                         std::map<std::string, TableDesc> &tables);
  bool HandleTopLevelDecl(clang::DeclGroupRef D) override;
 private:
  BTypeVisitor visitor_;
};

// Create a B program in 2 phases (everything else is normal C frontend):
// 1. Catch the map declarations and open the fd's
// 2. Capture the IR
class BFrontendAction : public clang::ASTFrontendAction {
 public:
  // Initialize with the output stream where the new source file contents
  // should be written.
  explicit BFrontendAction(llvm::raw_ostream &os);

  // Called by clang when the AST has been completed, here the output stream
  // will be flushed.
  void EndSourceFileAction() override;

  std::unique_ptr<clang::ASTConsumer>
      CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) override;

  // take ownership of the table-to-fd mapping data structure
  std::unique_ptr<std::map<std::string, TableDesc>> take_tables() { return move(tables_); }
 private:
  std::unique_ptr<clang::Rewriter> rewriter_;
  llvm::raw_ostream &os_;
  std::unique_ptr<std::map<std::string, TableDesc>> tables_;
};

}  // namespace visitor

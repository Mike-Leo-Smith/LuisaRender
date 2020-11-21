//
// Created by Mike Smith on 2020/7/31.
//

#pragma once

#include <ostream>
#include <fmt/format.h>

#include <compute/function.h>
#include <core/platform.h>
#include <core/concepts.h>

#include "expression.h"
#include "statement.h"

namespace luisa::compute {
class Device;
}

namespace luisa::compute::dsl {

class CodeScratch {

public:
    static constexpr auto scratch_size = 1024ul * 1024ul;
    using Scratch = std::array<char, scratch_size>;

private:
    std::unique_ptr<Scratch> _scratch;
    size_t _offset;
    
    template<typename FmtStr, typename ...Args>
    CodeScratch &_format(FmtStr &&format, Args &&...args) {
        auto result = fmt::format_to_n(_scratch->data() + _offset, scratch_size - std::min(_offset, scratch_size), std::forward<FmtStr>(format), std::forward<Args>(args)...);
        LUISA_EXCEPTION_IF_NOT(_offset + result.size < scratch_size, "Code generation scratch buffer overflows.");
        _offset += result.size;
        return *this;
    }

public:
    CodeScratch() noexcept : _scratch{std::make_unique<Scratch>()}, _offset{0u} {}
    CodeScratch &operator<<(float x) {
        LUISA_EXCEPTION_IF(std::isnan(x), "NaN encountered in codegen.");
        if (std::isinf(x)) { return _format("(1.0f / {}0.0f)", x > 0.0f ? "+" : "-"); }
        return _format("{:a}f", x);
    }
    CodeScratch &operator<<(bool x) { return _format(x ? "true" : "false"); }
//    CodeScratch &operator<<(char x) { return _format("static_cast<char>({})", static_cast<int>(x)); }
    CodeScratch &operator<<(uchar x) { return _format("static_cast<uchar>({})", static_cast<uint>(x)); }
    CodeScratch &operator<<(short x) { return _format("static_cast<short>({})", static_cast<int>(x)); }
    CodeScratch &operator<<(ushort x) { return _format("static_cast<ushort>({})", static_cast<uint>(x)); }
    CodeScratch &operator<<(int x) { return _format("{}", x); }
    CodeScratch &operator<<(uint x) { return _format("{}u", x); }
    CodeScratch &operator<<(const char *s) { return _format("{}", s); }
    CodeScratch &operator<<(const std::string &s) { return _format("{}", s); }
    CodeScratch &operator<<(std::string_view s) { return _format("{}", s); }
    
    [[nodiscard]] std::string_view view() const noexcept { return {_scratch->data(), std::min(_offset, scratch_size)}; }
    void clear() { _offset = 0u; }
};

class Codegen : Noncopyable {

protected:
    CodeScratch &_os;

public:
    explicit Codegen(CodeScratch &os) noexcept : _os{os} {}
    virtual ~Codegen() noexcept = default;
    virtual void emit(const Function &function) = 0;
};

// Example codegen for C++
class CppCodegen : public Codegen, public ExprVisitor, public StmtVisitor {

protected:
    int32_t _indent{0};
    bool _after_else{false};
    
    virtual void _emit_indent();
    virtual void _emit_function_decl(const Function &f);
    virtual void _emit_function_body(const Function &f);
    virtual void _emit_struct_decl(const TypeDesc *desc);
    virtual void _emit_variable(const Variable *v);
    virtual void _emit_type(const TypeDesc *desc);
    virtual void _emit_builtin_function_name(const std::string &func);

public:
    explicit CppCodegen(CodeScratch &os) noexcept : Codegen{os} {}
    ~CppCodegen() noexcept override = default;
    
    void emit(const Function &f) override;
    void visit(const UnaryExpr *unary_expr) override;
    void visit(const BinaryExpr *binary_expr) override;
    void visit(const MemberExpr *member_expr) override;
    void visit(const ValueExpr *literal_expr) override;
    void visit(const CallExpr *func_expr) override;
    void visit(const CastExpr *cast_expr) override;
    void visit(const TextureExpr *tex_expr) override;
    void visit(const EmptyStmt *stmt) override;
    void visit(const BreakStmt *stmt) override;
    void visit(const ContinueStmt *stmt) override;
    void visit(const ReturnStmt *stmt) override;
    void visit(const ScopeStmt *scope_stmt) override;
    void visit(const DeclareStmt *declare_stmt) override;
    void visit(const IfStmt *if_stmt) override;
    void visit(const WhileStmt *while_stmt) override;
    void visit(const ExprStmt *expr_stmt) override;
    void visit(const SwitchStmt *switch_stmt) override;
    void visit(const SwitchCaseStmt *case_stmt) override;
    void visit(const SwitchDefaultStmt *default_stmt) override;
    void visit(const AssignStmt *assign_stmt) override;
};

}

#include "mlta/engine.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

using namespace mlta;

namespace {
int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::shared_ptr<Node> lit(const std::string& id, Value v) {
    auto n = std::make_shared<Node>(id, id, NodeKind::Literal);
    n->literal = std::move(v);
    return n;
}

void testLiteral() {
    Engine engine;
    Context context;
    Node root("root", "root", NodeKind::Master);
    root.children.push_back(lit("hello", std::string{"world"}));
    auto result = engine.evaluate(root, context);
    check(result.success, "literal tree should execute");
    check(std::get<std::string>(result.value) == "world", "literal result should be returned");
}

void testBooleanAndShortCircuit() {
    Engine engine;
    Context context;
    Node root("root", "root", NodeKind::Operation);
    root.op = Operator::And;
    root.children.push_back(lit("false", false));
    auto missing = std::make_shared<Node>("missing", "missing", NodeKind::Variable);
    missing->variableName = "not-defined";
    root.children.push_back(missing);
    auto result = engine.evaluate(root, context);
    check(result.success, "AND should short-circuit before undefined variable");
    check(std::get<bool>(result.value) == false, "AND result should be false");
}

void testBranchThenAndElse() {
    Engine engine;
    Context context;
    Node root("root", "root", NodeKind::Branch);
    root.children.push_back(lit("condition", true));
    root.children.push_back(lit("then", std::int64_t{1}));
    root.children.push_back(lit("else", std::int64_t{2}));
    auto result = engine.evaluate(root, context);
    check(result.success, "branch should execute");
    check(std::get<std::int64_t>(result.value) == 1, "true condition should select then branch");
    check(std::find(context.trace.begin(), context.trace.end(), "else") == context.trace.end(),
          "unused ELSE branch must not execute");
}

void testUndefinedVariable() {
    Engine engine;
    Context context;
    Node root("ref", "ref", NodeKind::Variable);
    root.variableName = "missing";
    auto result = engine.evaluate(root, context);
    check(!result.success, "undefined variable should fail");
    check(!context.errors.empty(), "undefined variable should be diagnosed");
}

void testDuplicateIds() {
    Engine engine;
    Context context;
    Node root("same", "root", NodeKind::Master);
    root.children.push_back(lit("same", true));
    auto result = engine.evaluate(root, context);
    check(!result.success, "duplicate IDs should fail validation");
}

void testDivideByZero() {
    Engine engine;
    Context context;
    Node root("div", "divide", NodeKind::Operation);
    root.op = Operator::Divide;
    root.children = {lit("a", std::int64_t{4}), lit("b", std::int64_t{0})};
    auto result = engine.evaluate(root, context);
    check(!result.success, "division by zero should fail");
}

void testDepthLimit() {
    Engine engine;
    Context context;
    context.limits.maxDepth = 1;
    Node root("root", "root", NodeKind::Master);
    auto child = std::make_shared<Node>("child", "child", NodeKind::Sequence);
    child->children.push_back(lit("grandchild", true));
    root.children.push_back(child);
    auto result = engine.evaluate(root, context);
    check(!result.success, "depth limit should reject deeply nested tree");
}

} // namespace

int main() {
    testLiteral();
    testBooleanAndShortCircuit();
    testBranchThenAndElse();
    testUndefinedVariable();
    testDuplicateIds();
    testDivideByZero();
    testDepthLimit();
    if (failures != 0) {
        std::cerr << failures << " test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All MLTA tests passed\n";
    return EXIT_SUCCESS;
}

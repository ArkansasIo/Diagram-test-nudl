#include "mlta/engine.hpp"

#include <iostream>
#include <memory>

using mlta::Node;
using mlta::NodeKind;
using mlta::Operator;

static std::shared_ptr<Node> literal(const std::string& id, mlta::Value value) {
    auto node = std::make_shared<Node>(id, id, NodeKind::Literal);
    node->literal = std::move(value);
    return node;
}

int main() {
    // MASTER: if (score >= 50) grant access else deny.
    // The compact starter supports > and <; use (score > 49) for this example.
    mlta::Context context;
    context.variables["score"] = std::int64_t{72};

    auto root = std::make_shared<Node>("master", "Master", NodeKind::Master);
    auto branch = std::make_shared<Node>("branch", "ScoreGate", NodeKind::Branch);
    auto condition = std::make_shared<Node>("condition", "score > 49", NodeKind::Operation);
    condition->op = Operator::Greater;
    auto score = std::make_shared<Node>("score-ref", "score", NodeKind::Variable);
    score->variableName = "score";
    condition->children = {score, literal("threshold", std::int64_t{49})};

    auto granted = std::make_shared<Node>("grant", "GrantAccess", NodeKind::Statement);
    granted->children.push_back(literal("granted-value", std::string{"ACCESS_GRANTED"}));
    auto denied = std::make_shared<Node>("deny", "DenyAccess", NodeKind::Statement);
    denied->children.push_back(literal("denied-value", std::string{"ACCESS_DENIED"}));

    branch->children = {condition, granted, denied};
    root->children.push_back(branch);

    mlta::Engine engine;
    const auto result = engine.evaluate(*root, context);
    if (!result.success) {
        std::cerr << "Execution failed at " << result.nodeId << ": " << result.error << '\n';
        for (const auto& error : context.errors) std::cerr << " - " << error << '\n';
        return 1;
    }

    std::cout << "Result: " << mlta::valueToString(result.value) << '\n';
    std::cout << "Trace:";
    for (const auto& id : context.trace) std::cout << ' ' << id;
    std::cout << '\n';
    return 0;
}

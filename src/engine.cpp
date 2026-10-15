#include "mlta/engine.hpp"

#include <cmath>
#include <limits>
#include <sstream>
#include <type_traits>
#include <utility>

namespace mlta {

Node::Node(std::string nodeId, std::string nodeName, NodeKind nodeKind)
    : id(std::move(nodeId)), name(std::move(nodeName)), kind(nodeKind) {}

Result Result::ok(Value value, std::string nodeId) {
    return {true, std::move(value), std::move(nodeId), {}};
}
Result Result::fail(std::string nodeId, std::string message) {
    return {false, false, std::move(nodeId), std::move(message)};
}

std::string valueToString(const Value& value) {
    return std::visit([](const auto& item) -> std::string {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::monostate>) return "null";
        else if constexpr (std::is_same_v<T, bool>) return item ? "true" : "false";
        else if constexpr (std::is_same_v<T, std::string>) return item;
        else {
            std::ostringstream out;
            out << item;
            return out.str();
        }
    }, value);
}

bool isBoolean(const Value& value) {
    return std::holds_alternative<bool>(value);
}

Result Engine::validate(const Node& root, Context& context) const {
    context.errors.clear();
    std::unordered_map<std::string, bool> ids;
    auto result = validateNode(root, context, 0, ids);
    context.visitedNodes = 0;
    return result;
}

Result Engine::validateNode(const Node& node, Context& context,
                            std::size_t depth,
                            std::unordered_map<std::string, bool>& ids) const {
    if (depth > context.limits.maxDepth) {
        const auto msg = "maximum tree depth exceeded at node '" + node.id + "'";
        context.errors.push_back(msg);
        return Result::fail(node.id, msg);
    }
    if (++context.visitedNodes > context.limits.maxNodes) {
        const auto msg = "maximum node count exceeded";
        context.errors.push_back(msg);
        return Result::fail(node.id, msg);
    }
    if (node.id.empty()) {
        const auto msg = "node identifier must not be empty";
        context.errors.push_back(msg);
        return Result::fail(node.id, msg);
    }
    if (ids.contains(node.id)) {
        const auto msg = "duplicate node identifier '" + node.id + "'";
        context.errors.push_back(msg);
        return Result::fail(node.id, msg);
    }
    ids.emplace(node.id, true);

    if (node.kind == NodeKind::Literal && !node.literal.has_value()) {
        const auto msg = "literal node '" + node.id + "' has no value";
        context.errors.push_back(msg);
        return Result::fail(node.id, msg);
    }
    if (node.kind == NodeKind::Variable && node.variableName.empty()) {
        const auto msg = "variable node '" + node.id + "' has no variable name";
        context.errors.push_back(msg);
        return Result::fail(node.id, msg);
    }
    for (const auto& child : node.children) {
        if (!child) {
            const auto msg = "null child under node '" + node.id + "'";
            context.errors.push_back(msg);
            return Result::fail(node.id, msg);
        }
        auto result = validateNode(*child, context, depth + 1, ids);
        if (!result.success) return result;
    }
    return Result::ok(true, node.id);
}

Result Engine::evaluate(const Node& root, Context& context) const {
    auto checked = validate(root, context);
    if (!checked.success) return checked;
    context.trace.clear();
    context.errors.clear();
    context.visitedNodes = 0;
    return evaluateNode(root, context, 0);
}

Result Engine::evaluateNode(const Node& node, Context& context,
                            std::size_t depth) const {
    if (depth > context.limits.maxDepth)
        return Result::fail(node.id, "maximum execution depth exceeded");
    if (++context.visitedNodes > context.limits.maxNodes)
        return Result::fail(node.id, "maximum execution node count exceeded");
    if (context.trace.size() < context.limits.maxTraceEntries)
        context.trace.push_back(node.id);

    if (!node.enabled) return Result::ok(true, node.id);

    switch (node.kind) {
    case NodeKind::Literal:
        return Result::ok(node.literal.value_or(Value{}), node.id);

    case NodeKind::Variable: {
        const auto it = context.variables.find(node.variableName);
        if (it == context.variables.end()) {
            const auto msg = "undefined variable '" + node.variableName + "'";
            context.errors.push_back(msg);
            return Result::fail(node.id, msg);
        }
        return Result::ok(it->second, node.id);
    }

    case NodeKind::Condition:
        if (node.children.size() != 1) {
            const auto msg = "condition node must have exactly one child";
            context.errors.push_back(msg);
            return Result::fail(node.id, msg);
        } else {
            auto condition = evaluateNode(*node.children.front(), context, depth + 1);
            if (!condition.success) return condition;
            if (!isBoolean(condition.value)) {
                const auto msg = "condition expression must evaluate to Boolean";
                context.errors.push_back(msg);
                return Result::fail(node.id, msg);
            }
            return Result::ok(condition.value, node.id);
        }

    case NodeKind::Branch: {
        if (node.children.empty()) return Result::ok(false, node.id);
        // Branch convention: child 0 is the Boolean condition, child 1 is body.
        // Optional child 2 is ELSE body, executed when the condition is false.
        if (node.children.size() < 2 || node.children.size() > 3) {
            const auto msg = "branch expects condition, then-body, and optional else-body";
            context.errors.push_back(msg);
            return Result::fail(node.id, msg);
        }
        auto condition = evaluateNode(*node.children[0], context, depth + 1);
        if (!condition.success) return condition;
        if (!isBoolean(condition.value)) {
            const auto msg = "branch condition must be Boolean";
            context.errors.push_back(msg);
            return Result::fail(node.id, msg);
        }
        if (std::get<bool>(condition.value))
            return evaluateNode(*node.children[1], context, depth + 1);
        if (node.children.size() == 3)
            return evaluateNode(*node.children[2], context, depth + 1);
        return Result::ok(false, node.id);
    }

    case NodeKind::Operation:
        return evaluateOperator(node, context, depth);

    case NodeKind::Array: {
        // Current Value is scalar-only; arrays execute children in order and
        // return the last child result. Array-valued Values are a future extension.
        Value last = true;
        for (const auto& child : node.children) {
            auto result = evaluateNode(*child, context, depth + 1);
            if (!result.success) {
                if (node.stopOnFailure) return result;
                context.errors.push_back(result.error);
            } else {
                last = result.value;
            }
        }
        return Result::ok(last, node.id);
    }

    case NodeKind::Master:
    case NodeKind::Sequence:
    case NodeKind::Statement:
    case NodeKind::Module:
    case NodeKind::Class:
    case NodeKind::Type: {
        Value last = true;
        for (const auto& child : node.children) {
            auto result = evaluateNode(*child, context, depth + 1);
            if (!result.success) {
                if (node.stopOnFailure) return result;
                context.errors.push_back(result.error);
                continue;
            }
            last = result.value;
        }
        return Result::ok(last, node.id);
    }
    }
    return Result::fail(node.id, "unsupported node kind");
}

Result Engine::evaluateOperator(const Node& node, Context& context,
                                std::size_t depth) const {
    if (node.op == Operator::Not) {
        if (node.children.size() != 1)
            return Result::fail(node.id, "NOT expects one operand");
        auto operand = evaluateNode(*node.children[0], context, depth + 1);
        if (!operand.success) return operand;
        if (!isBoolean(operand.value))
            return Result::fail(node.id, "NOT operand must be Boolean");
        return Result::ok(!std::get<bool>(operand.value), node.id);
    }

    if (node.op == Operator::And || node.op == Operator::Or) {
        const bool isAnd = node.op == Operator::And;
        for (const auto& child : node.children) {
            auto operand = evaluateNode(*child, context, depth + 1);
            if (!operand.success) return operand;
            if (!isBoolean(operand.value))
                return Result::fail(node.id, "Boolean operands must be Boolean");
            const bool value = std::get<bool>(operand.value);
            if (isAnd && !value) return Result::ok(false, node.id);
            if (!isAnd && value) return Result::ok(true, node.id);
        }
        return Result::ok(isAnd, node.id);
    }

    if (node.children.size() != 2)
        return Result::fail(node.id, "binary operator expects two operands");

    auto left = evaluateNode(*node.children[0], context, depth + 1);
    if (!left.success) return left;
    auto right = evaluateNode(*node.children[1], context, depth + 1);
    if (!right.success) return right;

    if (node.op == Operator::Equal || node.op == Operator::NotEqual) {
        if (left.value.index() != right.value.index())
            return Result::fail(node.id, "equality operands must have matching types");
        const bool equal = left.value == right.value;
        return Result::ok(node.op == Operator::Equal ? equal : !equal, node.id);
    }

    const auto numeric = [](const Value& value) -> std::optional<long double> {
        if (const auto* v = std::get_if<std::int64_t>(&value)) return *v;
        if (const auto* v = std::get_if<double>(&value)) return *v;
        return std::nullopt;
    };

    const auto a = numeric(left.value);
    const auto b = numeric(right.value);
    if (!a || !b)
        return Result::fail(node.id, "operator requires numeric operands");

    switch (node.op) {
    case Operator::Greater: return Result::ok(*a > *b, node.id);
    case Operator::Less: return Result::ok(*a < *b, node.id);
    case Operator::Add: return Result::ok(static_cast<double>(*a + *b), node.id);
    case Operator::Subtract: return Result::ok(static_cast<double>(*a - *b), node.id);
    case Operator::Multiply: return Result::ok(static_cast<double>(*a * *b), node.id);
    case Operator::Divide:
        if (*b == 0.0L) return Result::fail(node.id, "division by zero");
        return Result::ok(static_cast<double>(*a / *b), node.id);
    default:
        return Result::fail(node.id, "unsupported operator");
    }
}

} // namespace mlta

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace mlta {

using Value = std::variant<std::monostate, bool, std::int64_t, double, std::string>;

enum class NodeKind {
    Master,
    Sequence,
    Statement,
    Condition,
    Branch,
    Module,
    Class,
    Type,
    Array,
    Literal,
    Variable,
    Operation
};

enum class Operator {
    None,
    And,
    Or,
    Not,
    Equal,
    NotEqual,
    Greater,
    Less,
    Add,
    Subtract,
    Multiply,
    Divide
};

struct Node {
    std::string id;
    std::string name;
    NodeKind kind{NodeKind::Statement};
    Operator op{Operator::None};
    std::optional<Value> literal;
    std::string variableName;
    std::string typeName;
    std::vector<std::shared_ptr<Node>> children;
    bool enabled{true};
    bool stopOnFailure{true};

    Node(std::string nodeId, std::string nodeName, NodeKind nodeKind);
};

struct Limits {
    std::size_t maxDepth{256};
    std::size_t maxNodes{100000};
    std::size_t maxTraceEntries{100000};
};

struct Context {
    std::unordered_map<std::string, Value> variables;
    std::vector<std::string> trace;
    std::vector<std::string> errors;
    Limits limits;
    std::size_t visitedNodes{0};
};

struct Result {
    bool success{false};
    Value value{};
    std::string nodeId;
    std::string error;

    static Result ok(Value value, std::string nodeId);
    static Result fail(std::string nodeId, std::string message);
};

class Engine {
public:
    Result validate(const Node& root, Context& context) const;
    Result evaluate(const Node& root, Context& context) const;

private:
    Result validateNode(const Node& node, Context& context,
                        std::size_t depth,
                        std::unordered_map<std::string, bool>& ids) const;
    Result evaluateNode(const Node& node, Context& context,
                        std::size_t depth) const;
    Result evaluateOperator(const Node& node, Context& context,
                            std::size_t depth) const;
};

std::string valueToString(const Value& value);
bool isBoolean(const Value& value);

} // namespace mlta

#pragma once

#include <vector>
#include <memory>

#include "json.hpp"

namespace opossum {

class AbstractLQPNode;
class AbstractJoinPlanPredicate;
class LQPColumnReference;

struct BaseJoinGraph final {
  static BaseJoinGraph from_joined_graphs(const BaseJoinGraph& left, const BaseJoinGraph& right);

  BaseJoinGraph() = default;
  BaseJoinGraph(const std::vector<std::shared_ptr<AbstractLQPNode>>& vertices, const std::vector<std::shared_ptr<const AbstractJoinPlanPredicate>>& predicates);

  std::shared_ptr<AbstractLQPNode> find_vertex(const LQPColumnReference& column_reference) const;

  std::string description() const;

  nlohmann::json to_json() const;
  static BaseJoinGraph from_json(const nlohmann::json& json);

  bool operator==(const BaseJoinGraph& rhs) const;

  std::vector<std::shared_ptr<AbstractLQPNode>> vertices;
  std::vector<std::shared_ptr<const AbstractJoinPlanPredicate>> predicates;
};

} // namespace opossum

namespace std {

template<>
struct hash<opossum::BaseJoinGraph> {
  size_t operator()(const opossum::BaseJoinGraph& join_graph) const;
};

}  // namespace std
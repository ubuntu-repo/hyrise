#pragma once

#include <optional>

#include "boost/functional/hash.hpp"

#include "abstract_expression2.hpp"
#include "logical_query_plan/lqp_column_reference.hpp"

namespace opossum {

/**
 * Expression type used in LQPs, using LQPColumnReferences to refer to Columns.
 * AbstractExpression handles all other possible contained types (literals, operators, ...).
 */
class LQPExpression : public AbstractExpression<LQPExpression> {
 public:
  static std::shared_ptr<LQPExpression> create_column(const LQPColumnReference& column_reference,
                                                      const std::optional<std::string>& alias = std::nullopt);

  static std::shared_ptr<LQPExpression> create_in(const LQPColumnReference& column_reference,
                                                  const std::vector<AllTypeVariant>& array);

  static std::vector<std::shared_ptr<LQPExpression>> create_columns(
      const std::vector<LQPColumnReference>& column_references,
      const std::optional<std::vector<std::string>>& aliases = std::nullopt);

  // Necessary for the AbstractExpression<T>::create_*() methods
  using AbstractExpression<LQPExpression>::AbstractExpression;

  const LQPColumnReference& column_reference() const;

  void set_column_reference(const LQPColumnReference& column_reference);

  std::string to_string(const std::optional<std::vector<std::string>>& input_column_names = std::nullopt,
                        bool is_root = true) const override;

  bool operator==(const LQPExpression& other) const;

  size_t hash() const override;

 protected:
  void _deep_copy_impl(const std::shared_ptr<LQPExpression>& copy) const override;

 private:
  std::optional<LQPColumnReference> _column_reference;
};
}  // namespace opossum
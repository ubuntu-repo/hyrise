#pragma once

#include "abstract_expression.hpp"

namespace opossum {

enum class LogicalOperator {
  And,
  Or,
  Not
};

class LogicalExpression : public AbstractExpression {
 public:
  LogicalExpression(const LogicalOperator logical_operator, const std::shared_ptr<AbstractExpression>& left_operand, const std::shared_ptr<AbstractExpression>& right_operand);

  /**
   * @defgroup Overrides for AbstractExpression
   * @{
   */
  bool deep_equals(const AbstractExpression& expression) const override;
  std::shared_ptr<AbstractExpression> deep_copy() const override;
  std::shared_ptr<AbstractExpression> deep_resolve_column_expressions() override;
  /**@}*/

  LogicalOperator logical_operator;
  std::shared_ptr<AbstractExpression> left_operand;
  std::shared_ptr<AbstractExpression> right_operand;
};

}  // namespace opossum

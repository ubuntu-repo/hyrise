#pragma once

#include "abstract_expression.hpp"

namespace opossum {

class AbstractLQPNode;

class SelectExpression : public AbstractExpression {
 public:
  SelectExpression(const std::shared_ptr<AbstractLQPNode>& lqp);

  /**
   * @defgroup Overrides for AbstractExpression
   * @{
   */
  bool deep_equals(const AbstractExpression& expression) const override;
  std::shared_ptr<AbstractExpression> deep_copy() const override;
  std::shared_ptr<AbstractExpression> deep_resolve_column_expressions() override;
  /**@}*/

  std::shared_ptr<AbstractLQPNode> lqp;
};

}  // namespace opossum

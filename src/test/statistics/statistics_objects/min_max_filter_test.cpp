#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base_test.hpp"
#include "gtest/gtest.h"

#include "utils/assert.hpp"

#include "statistics/statistics_objects/min_max_filter.hpp"
#include "types.hpp"

namespace opossum {

template <typename T>
class MinMaxFilterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    _values = pmr_vector<T>{-1000, 2, 3, 4, 7, 8, 10, 17, 123456};
    _min_value = *std::min_element(std::begin(_values), std::end(_values));
    _max_value = *std::max_element(std::begin(_values), std::end(_values));
    _in_between = static_cast<T>(_min_value + 0.5 * (_max_value - _min_value));  // value in between the min and max
    _in_between2 =
        static_cast<T>(_in_between + 0.5 * (_max_value - _in_between));  // value in between _in_between and max
    _before_range = _min_value - 1;                                      // value smaller than the minimum
    _after_range = _max_value + 1;                                       // value larger than the maximum
  }

  pmr_vector<T> _values;
  T _before_range, _min_value, _max_value, _after_range, _in_between, _in_between2;
};

// the test data for strings needs to be handled differently from numerics
template <>
class MinMaxFilterTest<pmr_string> : public ::testing::Test {
 protected:
  void SetUp() override {
    _values = pmr_vector<pmr_string>{"aa", "bb", "b", "bbbbba", "bbbbbb", "bbbbbc", "c"};
    _min_value = *std::min_element(std::begin(_values), std::end(_values));
    _max_value = *std::max_element(std::begin(_values), std::end(_values));
    _in_between = "ba";   // value in between the min and max
    _in_between2 = "bm";  // value in between _in_between and max
    _before_range = "a";  // value smaller/before than the minimum
    _after_range = "cc";  // value larger/beyond than the maximum
  }

  pmr_vector<pmr_string> _values;
  pmr_string _before_range, _min_value, _max_value, _after_range, _in_between, _in_between2;
};

using FilterTypes = ::testing::Types<int, float, double, pmr_string>;
TYPED_TEST_CASE(MinMaxFilterTest, FilterTypes, );  // NOLINT(whitespace/parens)

TYPED_TEST(MinMaxFilterTest, CanPruneOnBounds) {
  auto filter = std::make_unique<MinMaxFilter<TypeParam>>(this->_values.front(), this->_values.back());

  for (const auto& value : this->_values) {
    EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::Equals, {value}).type,
              EstimateType::MatchesApproximately);
  }

  // for the predicate condition of <, we expect only values smaller or equal to the minimum to be prunable
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::LessThan, {this->_before_range}).type,
            EstimateType::MatchesNone);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::LessThan, {this->_min_value}).type,
            EstimateType::MatchesNone);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::LessThan, {this->_in_between}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::LessThan, {this->_max_value}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::LessThan, {this->_after_range}).type,
            EstimateType::MatchesApproximately);

  // for the predicate condition of <=, we expect only values smaller than the minimum to be prunable
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::LessThanEquals, {this->_before_range}).type,
            EstimateType::MatchesNone);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::LessThanEquals, {this->_min_value}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::LessThanEquals, {this->_in_between}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::LessThanEquals, {this->_max_value}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::LessThanEquals, {this->_after_range}).type,
            EstimateType::MatchesApproximately);

  // for the predicate condition of ==, we expect only values outside the max/max range to be prunable
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::Equals, {this->_before_range}).type,
            EstimateType::MatchesNone);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::Equals, {this->_min_value}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::Equals, {this->_in_between}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::Equals, {this->_max_value}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::Equals, {this->_after_range}).type,
            EstimateType::MatchesNone);

  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::GreaterThanEquals, {this->_before_range}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::GreaterThanEquals, {this->_min_value}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::GreaterThanEquals, {this->_in_between}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::GreaterThanEquals, {this->_max_value}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::GreaterThanEquals, {this->_after_range}).type,
            EstimateType::MatchesNone);

  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::GreaterThan, {this->_before_range}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::GreaterThan, {this->_min_value}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::GreaterThan, {this->_in_between}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::GreaterThan, {this->_max_value}).type,
            EstimateType::MatchesNone);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::GreaterThan, {this->_after_range}).type,
            EstimateType::MatchesNone);

  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::IsNull, NULL_VALUE).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::IsNull, {this->_in_between}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::IsNull, {this->_min_value}, {this->_in_between}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::IsNotNull, NULL_VALUE).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::IsNotNull, {this->_in_between}).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::IsNotNull, {this->_min_value}, {this->_in_between}).type,
            EstimateType::MatchesApproximately);

  // as null values are not comparable, we never prune them
  EXPECT_EQ(filter->estimate_cardinality(PredicateCondition::IsNull, {this->_in_between}).type,
            EstimateType::MatchesApproximately);
}

TYPED_TEST(MinMaxFilterTest, Sliced) {
  auto new_filter = std::shared_ptr<MinMaxFilter<TypeParam>>{};

  const auto filter = std::make_unique<MinMaxFilter<TypeParam>>(this->_values.front(), this->_values.back());

  new_filter =
      std::static_pointer_cast<MinMaxFilter<TypeParam>>(filter->sliced(PredicateCondition::Equals, this->_in_between));

  // New filter should have _in_between as min and max.
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::LessThan, this->_in_between).type,
            EstimateType::MatchesNone);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::LessThanEquals, this->_in_between).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::GreaterThanEquals, this->_in_between).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::GreaterThan, this->_in_between).type,
            EstimateType::MatchesNone);

  new_filter = std::static_pointer_cast<MinMaxFilter<TypeParam>>(
      filter->sliced(PredicateCondition::NotEquals, this->_in_between));

  // Should be the same filter.
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::LessThan, this->_values.front()).type,
            EstimateType::MatchesNone);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::LessThanEquals, this->_values.front()).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::GreaterThanEquals, this->_values.back()).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::GreaterThan, this->_values.back()).type,
            EstimateType::MatchesNone);

  new_filter = std::static_pointer_cast<MinMaxFilter<TypeParam>>(
      filter->sliced(PredicateCondition::LessThanEquals, this->_in_between));

  // New filter should start at same value as before and end at value _in_between.
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::LessThan, this->_values.front()).type,
            EstimateType::MatchesNone);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::LessThanEquals, this->_values.front()).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::GreaterThanEquals, this->_in_between).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::GreaterThan, this->_in_between).type,
            EstimateType::MatchesNone);

  new_filter = std::static_pointer_cast<MinMaxFilter<TypeParam>>(
      filter->sliced(PredicateCondition::GreaterThanEquals, this->_in_between));

  // New filter should start at value _in_between and end at same value as before.
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::LessThan, this->_in_between).type,
            EstimateType::MatchesNone);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::LessThanEquals, this->_in_between).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::GreaterThanEquals, this->_values.back()).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::GreaterThan, this->_values.back()).type,
            EstimateType::MatchesNone);

  new_filter = std::static_pointer_cast<MinMaxFilter<TypeParam>>(
      filter->sliced(PredicateCondition::Between, this->_in_between, this->_in_between2));

  // New filter should start at _in_between and end at _in_between2.
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::LessThan, this->_in_between).type,
            EstimateType::MatchesNone);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::LessThanEquals, this->_in_between).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::GreaterThanEquals, this->_in_between2).type,
            EstimateType::MatchesApproximately);
  EXPECT_EQ(new_filter->estimate_cardinality(PredicateCondition::GreaterThan, this->_in_between2).type,
            EstimateType::MatchesNone);
}

TYPED_TEST(MinMaxFilterTest, SliceWithPredicateReturnsNullptr) {
  const auto filter = std::make_unique<MinMaxFilter<TypeParam>>(this->_values.front(), this->_values.back());

  EXPECT_EQ(filter->sliced(PredicateCondition::LessThan, this->_values.front()), nullptr);
  EXPECT_NE(filter->sliced(PredicateCondition::LessThanEquals, this->_values.front()), nullptr);
  EXPECT_NE(filter->sliced(PredicateCondition::GreaterThanEquals, this->_values.back()), nullptr);
  EXPECT_EQ(filter->sliced(PredicateCondition::GreaterThan, this->_values.back()), nullptr);
}

}  // namespace opossum
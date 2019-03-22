#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base_test.hpp"
#include "gtest/gtest.h"

#include "utils/assert.hpp"

#include "statistics/chunk_statistics/range_filter.hpp"
#include "types.hpp"

namespace opossum {

template <typename T>
class RangeFilterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Manually created vector. Largest exlusive gap (only gap when gap_count == 1) will
    // be 103-123456, second largest -1000 to 2, third 17-100.
    _values = pmr_vector<T>{-1000, 2, 3, 4, 7, 8, 10, 17, 100, 101, 102, 103, 123456};

    _min_value = *std::min_element(std::begin(_values), std::end(_values));
    _max_value = *std::max_element(std::begin(_values), std::end(_values));

    // `_value_in_gap` in a value in the largest gap of the test data.
    _value_in_gap = T{1024};

    _value_smaller_than_minimum = _min_value - 1;  // value smaller than the minimum
    _value_larger_than_maximum = _max_value + 1;   // value larger than the maximum
  }

  pmr_vector<T> _values;
  T _value_smaller_than_minimum, _min_value, _max_value, _value_larger_than_maximum, _value_in_gap;
};

using FilterTypes = ::testing::Types<int, float, double>;
TYPED_TEST_CASE(RangeFilterTest, FilterTypes, );  // NOLINT(whitespace/parens)

TYPED_TEST(RangeFilterTest, ValueRangeTooLarge) {
  const auto lowest = std::numeric_limits<TypeParam>::lowest();
  const auto max = std::numeric_limits<TypeParam>::max();
  // Create vector with a huge gap in the middle whose length exceeds the type's limits.
  const pmr_vector<TypeParam> test_vector{static_cast<TypeParam>(0.9 * lowest), static_cast<TypeParam>(0.8 * lowest),
                                          static_cast<TypeParam>(0.8 * max), static_cast<TypeParam>(0.9 * max)};

  // The filter will not create 5 ranges due to potential overflow problems when calculating
  // distances. In this case, only a filter with a single range is built.
  auto filter = RangeFilter<TypeParam>::build_filter(test_vector, 5);
  // Having only one range means the filter cannot prune 0 right in the largest gap.
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, static_cast<TypeParam>(0)));
  // Nonetheless, the filter should prune values outside the single range.
  EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, static_cast<TypeParam>(lowest * 0.95)));
}

TYPED_TEST(RangeFilterTest, ThrowOnUnsortedData) {
  if (!HYRISE_DEBUG) GTEST_SKIP();

  const pmr_vector<TypeParam> test_vector{std::numeric_limits<TypeParam>::max(),
                                          std::numeric_limits<TypeParam>::lowest()};

  // Additional parantheses needed for template macro expansion.
  EXPECT_THROW((RangeFilter<TypeParam>::build_filter(test_vector, 5)), std::logic_error);
}

// a single range is basically a min/max filter
TYPED_TEST(RangeFilterTest, SingleRange) {
  const auto filter = RangeFilter<TypeParam>::build_filter(this->_values, 1);

  for (const auto& value : this->_values) {
    EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, TypeParam{value}));
  }

  // testing for interval bounds
  EXPECT_TRUE(filter->can_prune(PredicateCondition::LessThan, TypeParam{this->_min_value}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::GreaterThan, TypeParam{this->_min_value}));

  // cannot prune values in between, even though non-existent
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, TypeParam{this->_value_in_gap}));

  EXPECT_FALSE(filter->can_prune(PredicateCondition::LessThanEquals, TypeParam{this->_max_value}));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::GreaterThan, TypeParam{this->_max_value}));

  EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, TypeParam{-3000}, TypeParam{-2000}));
}

// create range filters with varying number of ranges/gaps
TYPED_TEST(RangeFilterTest, MultipleRanges) {
  const auto first_gap_min = TypeParam{104};
  const auto first_gap_max = TypeParam{123455};

  const auto second_gap_min = TypeParam{-999};
  const auto second_gap_max = TypeParam{1};

  const auto third_gap_min = TypeParam{18};
  const auto third_gap_max = TypeParam{99};

  {
    const auto filter = RangeFilter<TypeParam>::build_filter(this->_values, 2);
    EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, this->_value_in_gap));
    EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, first_gap_min));
    EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, first_gap_min, first_gap_max));

    EXPECT_FALSE(filter->can_prune(PredicateCondition::Between, second_gap_min, second_gap_max));
    EXPECT_FALSE(filter->can_prune(PredicateCondition::Between, third_gap_min, third_gap_max));
  }
  {
    const auto filter = RangeFilter<TypeParam>::build_filter(this->_values, 3);
    EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, this->_value_in_gap));
    EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, first_gap_min));
    EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, first_gap_min, first_gap_max));
    EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, second_gap_min));
    EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, second_gap_min, second_gap_max));

    EXPECT_FALSE(filter->can_prune(PredicateCondition::Between, third_gap_min, third_gap_max));
  }
  // starting with 4 ranges, all tested gaps should be covered
  for (auto range_count : {4, 5, 100, 1'000}) {
    {
      const auto filter = RangeFilter<TypeParam>::build_filter(this->_values, range_count);
      EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, this->_value_in_gap));
      EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, first_gap_min));
      EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, first_gap_min, first_gap_max));
      EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, second_gap_min));
      EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, second_gap_min, second_gap_max));
      EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, third_gap_min));
      EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, third_gap_min, third_gap_max));
    }
  }
  {
    if (!HYRISE_DEBUG) GTEST_SKIP();

    // Throw when range filter shall include 0 range values.
    EXPECT_THROW((RangeFilter<TypeParam>::build_filter(this->_values, 0)), std::logic_error);
  }
}

// create more ranges than distinct values in the test data
TYPED_TEST(RangeFilterTest, MoreRangesThanValues) {
  const auto filter = RangeFilter<TypeParam>::build_filter(this->_values, 10'000);

  for (const auto& value : this->_values) {
    EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, {value}));
  }

  // testing for interval bounds
  EXPECT_TRUE(filter->can_prune(PredicateCondition::LessThan, TypeParam{this->_min_value}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::GreaterThan, TypeParam{this->_min_value}));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, TypeParam{this->_value_in_gap}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::LessThanEquals, TypeParam{this->_max_value}));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::GreaterThan, TypeParam{this->_max_value}));
}

// this test checks the correct pruning on the bounds (min/max) of the test data for various predicate conditions
// for better understanding, see min_max_filter_test.cpp
TYPED_TEST(RangeFilterTest, CanPruneOnBounds) {
  const auto filter = RangeFilter<TypeParam>::build_filter(this->_values);

  for (const auto& value : this->_values) {
    EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, {value}));
  }

  EXPECT_TRUE(filter->can_prune(PredicateCondition::LessThan, {this->_value_smaller_than_minimum}));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::LessThan, {this->_min_value}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::LessThan, {this->_value_in_gap}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::LessThan, {this->_max_value}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::LessThan, {this->_value_larger_than_maximum}));

  EXPECT_TRUE(filter->can_prune(PredicateCondition::LessThanEquals, {this->_value_smaller_than_minimum}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::LessThanEquals, {this->_min_value}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::LessThanEquals, {this->_value_in_gap}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::LessThanEquals, {this->_max_value}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::LessThanEquals, {this->_value_larger_than_maximum}));

  EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, {this->_value_smaller_than_minimum}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, {this->_min_value}));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, {this->_value_in_gap}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, {this->_max_value}));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, {this->_value_larger_than_maximum}));

  EXPECT_FALSE(filter->can_prune(PredicateCondition::GreaterThanEquals, {this->_value_smaller_than_minimum}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::GreaterThanEquals, {this->_min_value}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::GreaterThanEquals, {this->_value_in_gap}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::GreaterThanEquals, {this->_max_value}));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::GreaterThanEquals, {this->_value_larger_than_maximum}));

  EXPECT_FALSE(filter->can_prune(PredicateCondition::GreaterThan, {this->_value_smaller_than_minimum}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::GreaterThan, {this->_min_value}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::GreaterThan, {this->_value_in_gap}));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::GreaterThan, {this->_max_value}));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::GreaterThan, {this->_value_larger_than_maximum}));
}

// Test larger value ranges.
TYPED_TEST(RangeFilterTest, Between) {
  const auto filter = RangeFilter<TypeParam>::build_filter(this->_values);

  // This one has bounds in gaps, but cannot prune.
  EXPECT_FALSE(
      filter->can_prune(PredicateCondition::Between, {this->_max_value - 1}, {this->_value_larger_than_maximum}));

  EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, TypeParam{-3000}, TypeParam{-2000}));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, TypeParam{-999}, TypeParam{1}));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, TypeParam{104}, TypeParam{1004}));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, TypeParam{10'000'000}, TypeParam{20'000'000}));

  EXPECT_FALSE(filter->can_prune(PredicateCondition::Between, TypeParam{-3000}, TypeParam{-500}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Between, TypeParam{101}, TypeParam{103}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Between, TypeParam{102}, TypeParam{1004}));

  // SQL's between is inclusive
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Between, TypeParam{103}, TypeParam{123456}));

  // TODO(bensk1): as soon as non-inclusive between predicates are implemented, testing
  // a non-inclusive between with the bounds exactly on the value bounds would be humongous:
  //  EXPECT_TRUE(filter->can_prune(PredicateCondition::BetweenNONINCLUSIVE, TypeParam{103}, TypeParam{123456}));
}

// Test larger value ranges.
TYPED_TEST(RangeFilterTest, LargeValueRange) {
  const auto lowest = std::numeric_limits<TypeParam>::lowest();
  const auto max = std::numeric_limits<TypeParam>::max();

  const pmr_vector<TypeParam> values{static_cast<TypeParam>(0.4 * lowest),  static_cast<TypeParam>(0.38 * lowest),
                                     static_cast<TypeParam>(0.36 * lowest), static_cast<TypeParam>(0.30 * lowest),
                                     static_cast<TypeParam>(0.28 * lowest), static_cast<TypeParam>(0.36 * max),
                                     static_cast<TypeParam>(0.38 * max),    static_cast<TypeParam>(0.4 * max)};

  const auto filter = RangeFilter<TypeParam>::build_filter(values, 3);

  // A filter with 3 ranges, has two gaps: (i) 0.28*lowest-0.36*max and (ii) 0.36*lowest-0.30*lowest
  EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, static_cast<TypeParam>(0.27 * lowest),
                                static_cast<TypeParam>(0.35 * max)));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::Between, static_cast<TypeParam>(0.35 * lowest),
                                static_cast<TypeParam>(0.31 * lowest)));

  EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, TypeParam{0}));  // in gap
  EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, static_cast<TypeParam>(0.5 * lowest)));
  EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, static_cast<TypeParam>(0.5 * max)));

  EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, values.front(), values[4]));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, values[5], values.back()));

  // As SQL-between is inclusive, this range cannot be pruned.
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, values[4], values[5]));

  EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, static_cast<TypeParam>(0.4 * lowest)));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, static_cast<TypeParam>(0.4 * max)));

  // With two gaps, the following should not exist.
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Between, static_cast<TypeParam>(0.4 * lowest),
                                 static_cast<TypeParam>(0.38 * lowest)));
}

// Test predicates which are not supported by the range filter
TEST(RangeFilterTest, DoNotPruneUnsupportedPredicates) {
  const pmr_vector<int> values{-1000, -900, 900, 1000};
  const auto filter = RangeFilter<int>::build_filter(values);

  EXPECT_FALSE(filter->can_prune(PredicateCondition::IsNull, {17}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Like, {17}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::NotLike, {17}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::In, {17}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::NotIn, {17}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::IsNull, {17}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::IsNotNull, {17}));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::IsNull, NULL_VALUE));
  EXPECT_FALSE(filter->can_prune(PredicateCondition::IsNotNull, NULL_VALUE));

  // For the default filter, the following value is prunable.
  EXPECT_TRUE(filter->can_prune(PredicateCondition::Equals, 1));
  // But malformed predicates are skipped intentionally and are thus not prunable
  EXPECT_FALSE(filter->can_prune(PredicateCondition::Equals, 1, NULL_VALUE));
}

}  // namespace opossum

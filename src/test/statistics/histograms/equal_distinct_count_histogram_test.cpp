#include <limits>
#include <memory>
#include <string>

#include "base_test.hpp"
#include "gtest/gtest.h"

#include "statistics/histograms/equal_distinct_count_histogram.hpp"
#include "statistics/histograms/generic_histogram.hpp"
#include "statistics/histograms/histogram_utils.hpp"
#include "utils/load_table.hpp"

namespace opossum {

class EqualDistinctCountHistogramTest : public BaseTest {
  void SetUp() override {
    _int_float4 = load_table("resources/test_data/tbl/int_float4.tbl");
    _float2 = load_table("resources/test_data/tbl/float2.tbl");
    _string2 = load_table("resources/test_data/tbl/string2.tbl");
  }

 protected:
  std::shared_ptr<Table> _int_float4;
  std::shared_ptr<Table> _float2;
  std::shared_ptr<Table> _string2;
};

TEST_F(EqualDistinctCountHistogramTest, FromSegmentString) {
  StringHistogramDomain default_domain;
  const auto default_domain_histogram = EqualDistinctCountHistogram<std::string>::from_segment(
      _string2->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 4u, default_domain);

  ASSERT_EQ(default_domain_histogram->bin_count(), 4u);
  EXPECT_EQ(default_domain_histogram->bin(BinID{0}), HistogramBin<std::string>("aa", "birne", 3, 3));
  EXPECT_EQ(default_domain_histogram->bin(BinID{1}), HistogramBin<std::string>("bla", "ttt", 4, 3));
  EXPECT_EQ(default_domain_histogram->bin(BinID{2}), HistogramBin<std::string>("uuu", "xxx", 4, 3));


  StringHistogramDomain reduced_histogram{'a', 'c', 9};
  const auto reduced_domain_histogram = EqualDistinctCountHistogram<std::string>::from_segment(
      _string2->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 4u, default_domain);

  std::cout << reduced_domain_histogram->description() << std::endl;

  ASSERT_EQ(default_domain_histogram->bin_count(), 4u);
  EXPECT_EQ(default_domain_histogram->bin(BinID{0}), HistogramBin<std::string>("aa", "birne", 3, 3));
  EXPECT_EQ(default_domain_histogram->bin(BinID{1}), HistogramBin<std::string>("bla", "ttt", 4, 3));
  EXPECT_EQ(default_domain_histogram->bin(BinID{2}), HistogramBin<std::string>("uuu", "xxx", 4, 3));
}

TEST_F(EqualDistinctCountHistogramTest, FromSegmentInt) {
  const auto hist = EqualDistinctCountHistogram<int32_t>::from_segment(
      _int_float4->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 2u);

  ASSERT_EQ(hist->bin_count(), 2u);
  EXPECT_EQ(hist->bin(BinID{0}), HistogramBin<int32_t>(12, 123, 2, 2));
  EXPECT_EQ(hist->bin(BinID{1}), HistogramBin<int32_t>(12345, 123456, 5, 2));
}

TEST_F(EqualDistinctCountHistogramTest, FromSegmentFloat) {
  auto hist =
      EqualDistinctCountHistogram<float>::from_segment(_float2->get_chunk(ChunkID{0})->get_segment(ColumnID{0}), 3u);

  ASSERT_EQ(hist->bin_count(), 3u);
  EXPECT_EQ(hist->bin(BinID{0}), HistogramBin<float>(0.5f, 2.2f, 4, 4));
  EXPECT_EQ(hist->bin(BinID{1}), HistogramBin<float>(2.5f, 3.3f, 6, 3));
  EXPECT_EQ(hist->bin(BinID{2}), HistogramBin<float>(3.6f, 6.1f, 4, 3));
}

}  // namespace opossum
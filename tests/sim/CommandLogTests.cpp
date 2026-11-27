#include <colony/sim/CommandLog.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace {

using colony::sim::CancelTask;
using colony::sim::CommandEntry;
using colony::sim::CommandLog;
using colony::sim::MarkDirective;
using colony::sim::SetBalance;
using colony::tasks::TaskId;
using colony::util::Cell;

class CommandLogTest : public ::testing::Test {
protected:
  void SetUp() override {
    temp_file_ =
        std::filesystem::temp_directory_path() / "colony_test_command_log.txt";
    std::error_code ec;
    std::filesystem::remove(temp_file_, ec);
    sample_log_.add(10, SetBalance{75});
    sample_log_.add(5, MarkDirective{Cell{1, 2}});
    sample_log_.add(10, CancelTask{TaskId{3}});
    sample_log_.add(0, MarkDirective{Cell{0, 0}});
  }

  void TearDown() override {
    std::error_code ec;
    std::filesystem::remove(temp_file_, ec);
  }
  CommandLog sample_log_{};
  std::filesystem::path temp_file_{};
};

TEST_F(CommandLogTest, DefaultConstructedLogIsEmpty) {
  CommandLog empty_log;
  EXPECT_TRUE(empty_log.empty());
  EXPECT_EQ(empty_log.size(), 0u);
  EXPECT_TRUE(empty_log.entries().empty());
  EXPECT_TRUE(empty_log.commands_for_tick(0).empty());
}

TEST_F(CommandLogTest, SortsByTickAndPreservesOrderWithinSameTick) {
  EXPECT_EQ(sample_log_.size(), 4u);
  EXPECT_FALSE(sample_log_.empty());
  const auto tick0 = sample_log_.commands_for_tick(0);
  ASSERT_EQ(tick0.size(), 1u);
  EXPECT_EQ(tick0[0].command_,
            (colony::sim::Command{MarkDirective{Cell{0, 0}}}));
  const auto tick5 = sample_log_.commands_for_tick(5);
  ASSERT_EQ(tick5.size(), 1u);
  EXPECT_EQ(tick5[0].command_,
            (colony::sim::Command{MarkDirective{Cell{1, 2}}}));
  const auto tick10 = sample_log_.commands_for_tick(10);
  ASSERT_EQ(tick10.size(), 2u);
  EXPECT_EQ(tick10[0].command_, (colony::sim::Command{SetBalance{75}}));
  EXPECT_EQ(tick10[1].command_, (colony::sim::Command{CancelTask{TaskId{3}}}));
  EXPECT_TRUE(sample_log_.commands_for_tick(999).empty());
}

TEST_F(CommandLogTest, ClearRemovesAllEntries) {
  ASSERT_EQ(sample_log_.size(), 4u);
  sample_log_.clear();
  EXPECT_TRUE(sample_log_.empty());
  EXPECT_EQ(sample_log_.size(), 0u);
  EXPECT_TRUE(sample_log_.commands_for_tick(10).empty());
}

TEST_F(CommandLogTest, RoundTripSerializationAndFileIO) {
  static_cast<void>(sample_log_.commands_for_tick(0));
  const std::string serialized = sample_log_.to_string();
  const auto from_str = CommandLog::from_string(serialized);
  ASSERT_TRUE(from_str.ok_);
  EXPECT_EQ(from_str.log_, sample_log_);
  ASSERT_TRUE(sample_log_.save_to_file(temp_file_));
  const auto from_file = CommandLog::load_from_file(temp_file_);
  ASSERT_TRUE(from_file.ok_);
  EXPECT_EQ(from_file.log_, sample_log_);
}

TEST_F(CommandLogTest, FromStringParsesValidCommandsCommentsAndWhitespace) {
  constexpr std::string_view script = R"(
    # Initial directives
    15   BALANCE   80   # prioritize food

      0 DIRECTIVE 3 4
    15 CANCEL 2
    # Trailing empty lines and spaces
  )";

  const auto res = CommandLog::from_string(script);
  ASSERT_TRUE(res.ok_) << "Unexpected error at line " << res.error_line_ << ": "
                       << res.error_message_;
  ASSERT_EQ(res.log_.size(), 3u);
  const auto entries = res.log_.entries();
  EXPECT_EQ(entries[0], (CommandEntry{0, MarkDirective{Cell{3, 4}}}));
  EXPECT_EQ(entries[1], (CommandEntry{15, SetBalance{80}}));
  EXPECT_EQ(entries[2], (CommandEntry{15, CancelTask{TaskId{2}}}));
}

TEST_F(CommandLogTest, FromStringEmptyOrCommentsOnlySucceeds) {
  const auto res =
      CommandLog::from_string("# only comment\n   \n\t# another\n");
  EXPECT_TRUE(res.ok_);
  EXPECT_TRUE(res.log_.empty());
}

TEST_F(CommandLogTest,
       FromStringReportsSyntaxAndArgumentErrorsWithLineNumbers) {
  {
    const auto res =
        CommandLog::from_string("0 DIRECTIVE 1 2\nabc BALANCE 50\n");
    EXPECT_FALSE(res.ok_);
    EXPECT_EQ(res.error_line_, 2u);
  }
  {
    const auto res = CommandLog::from_string("10\n");
    EXPECT_FALSE(res.ok_);
    EXPECT_EQ(res.error_line_, 1u);
  }
  {
    const auto res = CommandLog::from_string("10 UNKNOWN_CMD 1\n");
    EXPECT_FALSE(res.ok_);
    EXPECT_EQ(res.error_line_, 1u);
  }
  {
    const auto res = CommandLog::from_string("10 DIRECTIVE 5\n");
    EXPECT_FALSE(res.ok_);
    EXPECT_EQ(res.error_line_, 1u);
  }
  {
    const auto res = CommandLog::from_string("10 DIRECTIVE -1 5\n");
    EXPECT_FALSE(res.ok_);
    EXPECT_EQ(res.error_line_, 1u);
  }
  {
    const auto res = CommandLog::from_string("10 CANCEL\n");
    EXPECT_FALSE(res.ok_);
    EXPECT_EQ(res.error_line_, 1u);
  }
  {
    const auto res = CommandLog::from_string("10 BALANCE not_a_num\n");
    EXPECT_FALSE(res.ok_);
    EXPECT_EQ(res.error_line_, 1u);
  }
  {
    const auto res = CommandLog::from_string("10 BALANCE 60 extra_token\n");
    EXPECT_FALSE(res.ok_);
    EXPECT_EQ(res.error_line_, 1u);
  }
}

TEST_F(CommandLogTest, LoadFromNonExistentFileFailsGracefully) {
  const auto res =
      CommandLog::load_from_file(std::filesystem::temp_directory_path() /
                                 "non_existent_colony_cmds_99999.txt");
  EXPECT_FALSE(res.ok_);
  EXPECT_EQ(res.error_line_, 0u);
}

} // namespace
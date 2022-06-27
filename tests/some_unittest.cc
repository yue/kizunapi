#include "testing/gtest/include/gtest/gtest.h"

namespace {

bool g_init = false;

}  // namespace

class SomeTest : public testing::Test {
 protected:
  void SetUp() override {
    g_init = true;
  }
};

TEST_F(SomeTest, Types) {
  EXPECT_TRUE(g_init);
}

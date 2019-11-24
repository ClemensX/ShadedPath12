#include "pch.h"
#include "test_misc.h"

TEST(TestTimer, Themes) {
  EXPECT_EQ(1, 1);
  EXPECT_TRUE(true);

  ThemedTimer* timer = ThemedTimer::getInstance();
  EXPECT_TRUE(timer != nullptr);

  timer->create("theme1", 1000);
  timer->create("theme2", 500);

  timer->add("theme1", 22000);
  timer->add("theme1", 44000);

  EXPECT_EQ(timer->usedSlots("theme1"), 2);

  Log("log entried for theme1:" << endl);
  timer->logEntries("theme1");
}


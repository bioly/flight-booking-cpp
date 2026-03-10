#include <gtest/gtest.h>
#include <sqlite3.h>

TEST(SqliteSmoke, CanOpenInMemoryAndExecuteDDL) {
  sqlite3* db = nullptr;
  ASSERT_EQ(sqlite3_open(":memory:", &db), SQLITE_OK);

  char* err = nullptr;
  int rc = sqlite3_exec(db, "CREATE TABLE t(x INTEGER);", nullptr, nullptr, &err);
  if (err) sqlite3_free(err);

  EXPECT_EQ(rc, SQLITE_OK);

  sqlite3_close(db);
}

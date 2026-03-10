#include "flight/infrastructure/sqlite_flight_repository.hpp"

#include "flight/domain/airport_code.hpp"
#include "flight/domain/flight.hpp"
#include "flight/domain/seat.hpp"

#include <sqlite3.h>

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace flight::infrastructure {

static void ok(int rc, sqlite3* db, const char* ctx) {
  if (rc == SQLITE_OK || rc == SQLITE_ROW || rc == SQLITE_DONE) return;
  throw std::runtime_error(std::string(ctx) + ": " + sqlite3_errmsg(db));
}

static std::int64_t to_epoch_seconds(flight::domain::Flight::time_point tp) {
  return std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
}

static flight::domain::Flight::time_point from_epoch_seconds(std::int64_t s) {
  return flight::domain::Flight::time_point{std::chrono::seconds{s}};
}

SqliteFlightRepository::SqliteFlightRepository(bool in_memory) {
  const char* name = in_memory ? ":memory:" : "flight_booking.db";
  ok(sqlite3_open(name, &db_), db_, "sqlite3_open");
  sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);
  prepare_schema();
}

SqliteFlightRepository::~SqliteFlightRepository() {
  if (db_) sqlite3_close(db_);
}

void SqliteFlightRepository::exec(const char* sql) const {
  char* err = nullptr;
  int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
  if (err) sqlite3_free(err);
  ok(rc, db_, "sqlite3_exec");
}

void SqliteFlightRepository::prepare_schema() {
  exec(R"sql(
    CREATE TABLE IF NOT EXISTS flights (
      flight_id        INTEGER PRIMARY KEY,
      origin           TEXT NOT NULL,
      destination      TEXT NOT NULL,
      departure_epoch  INTEGER NOT NULL,
      rows             INTEGER NOT NULL,
      seats_per_row    INTEGER NOT NULL
    );
  )sql");

  exec(R"sql(
    CREATE TABLE IF NOT EXISTS booked_seats (
      flight_id    INTEGER NOT NULL,
      seat_row     INTEGER NOT NULL,
      seat_letter  TEXT NOT NULL,
      PRIMARY KEY (flight_id, seat_row, seat_letter),
      FOREIGN KEY (flight_id) REFERENCES flights(flight_id) ON DELETE CASCADE
    );
  )sql");
}

void SqliteFlightRepository::upsert(flight::domain::Flight flight) {
  std::lock_guard<std::mutex> lock(mu_);

  const char* sql =
      "INSERT OR REPLACE INTO flights(flight_id, origin, destination, departure_epoch, rows, seats_per_row) "
      "VALUES(?,?,?,?,?,?);";

  sqlite3_stmt* st = nullptr;
  ok(sqlite3_prepare_v2(db_, sql, -1, &st, nullptr), db_, "prepare upsert flight");

  sqlite3_bind_int64(st, 1, static_cast<sqlite3_int64>(flight.id().value()));
  sqlite3_bind_text(st, 2, flight.origin().value().c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(st, 3, flight.destination().value().c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int64(st, 4, static_cast<sqlite3_int64>(to_epoch_seconds(flight.departure())));
  sqlite3_bind_int(st, 5, static_cast<int>(flight.rows()));
  sqlite3_bind_int(st, 6, static_cast<int>(flight.seats_per_row()));

  ok(sqlite3_step(st), db_, "step upsert flight");
  sqlite3_finalize(st);

  // Important: v1 doesn't upsert booked seats, because Flight doesn't expose them.
  // Booked seats are persisted via try_book_seat(). get/search reconstruct by querying booked_seats.
}

std::optional<flight::domain::Flight> SqliteFlightRepository::get(flight::domain::FlightId id) const {
  std::lock_guard<std::mutex> lock(mu_);

  const char* exists_sql =
      "SELECT 1 FROM flights WHERE flight_id=?;";

  sqlite3_stmt* st = nullptr;
  ok(sqlite3_prepare_v2(db_, exists_sql, -1, &st, nullptr), db_, "prepare exists get");
  sqlite3_bind_int64(st, 1, static_cast<sqlite3_int64>(id.value()));

  const int rc = sqlite3_step(st);
  sqlite3_finalize(st);

  if (rc != SQLITE_ROW) {
    return std::nullopt;
  }

  return load_flight_by_id_locked(id);
}

flight::domain::Flight
SqliteFlightRepository::load_flight_by_id_locked(flight::domain::FlightId id) const {
  const char* sql =
      "SELECT origin, destination, departure_epoch, rows, seats_per_row "
      "FROM flights WHERE flight_id=?;";

  sqlite3_stmt* st = nullptr;
  ok(sqlite3_prepare_v2(db_, sql, -1, &st, nullptr), db_, "prepare load flight");
  sqlite3_bind_int64(st, 1, static_cast<sqlite3_int64>(id.value()));

  const int rc = sqlite3_step(st);
  if (rc != SQLITE_ROW) {
    sqlite3_finalize(st);
    throw std::runtime_error("Flight not found while loading by id");
  }

  const std::string origin = reinterpret_cast<const char*>(sqlite3_column_text(st, 0));
  const std::string dest   = reinterpret_cast<const char*>(sqlite3_column_text(st, 1));
  const auto dep_epoch     = static_cast<std::int64_t>(sqlite3_column_int64(st, 2));
  const auto rows          = static_cast<std::uint16_t>(sqlite3_column_int(st, 3));
  const auto spr           = static_cast<std::uint8_t>(sqlite3_column_int(st, 4));

  sqlite3_finalize(st);

  flight::domain::Flight flight{
      id,
      flight::domain::AirportCode(origin),
      flight::domain::AirportCode(dest),
      from_epoch_seconds(dep_epoch),
      rows,
      spr};

  const char* sql2 =
      "SELECT seat_row, seat_letter FROM booked_seats WHERE flight_id=?;";

  sqlite3_stmt* st2 = nullptr;
  ok(sqlite3_prepare_v2(db_, sql2, -1, &st2, nullptr), db_, "prepare load seats");
  sqlite3_bind_int64(st2, 1, static_cast<sqlite3_int64>(id.value()));

  while (sqlite3_step(st2) == SQLITE_ROW) {
    const auto r = static_cast<std::uint16_t>(sqlite3_column_int(st2, 0));
    const std::string letter = reinterpret_cast<const char*>(sqlite3_column_text(st2, 1));
    const char c = letter.empty() ? 'A' : letter[0];
    flight.book_seat(flight::domain::Seat{r, c});
  }

  sqlite3_finalize(st2);
  return flight;
}

std::vector<flight::domain::Flight>
SqliteFlightRepository::search(const flight::application::FlightSearchCriteria& criteria) const {
  std::lock_guard<std::mutex> lock(mu_);

  const char* sql =
      "SELECT flight_id FROM flights WHERE origin=? AND destination=? ORDER BY flight_id ASC;";

  sqlite3_stmt* st = nullptr;
  ok(sqlite3_prepare_v2(db_, sql, -1, &st, nullptr), db_, "prepare search");
  sqlite3_bind_text(st, 1, criteria.origin.value().c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(st, 2, criteria.destination.value().c_str(), -1, SQLITE_TRANSIENT);

  std::vector<flight::domain::Flight> out;
  while (sqlite3_step(st) == SQLITE_ROW) {
    const auto fid = static_cast<std::uint64_t>(sqlite3_column_int64(st, 0));
    out.push_back(load_flight_by_id_locked(flight::domain::FlightId{fid}));
  }

  sqlite3_finalize(st);
  return out;
}

bool SqliteFlightRepository::try_book_seat(flight::domain::FlightId flight_id, const flight::domain::Seat& seat) {
  std::lock_guard<std::mutex> lock(mu_);

  // Validate against flight config (rows, seats_per_row)
  const char* sql =
      "SELECT rows, seats_per_row FROM flights WHERE flight_id=?;";
  sqlite3_stmt* st = nullptr;
  ok(sqlite3_prepare_v2(db_, sql, -1, &st, nullptr), db_, "prepare seat validate");
  sqlite3_bind_int64(st, 1, static_cast<sqlite3_int64>(flight_id.value()));

  const int rc = sqlite3_step(st);
  if (rc != SQLITE_ROW) {
    sqlite3_finalize(st);
    return false; // flight not found
  }

  const auto rows = static_cast<std::uint16_t>(sqlite3_column_int(st, 0));
  const auto spr  = static_cast<std::uint8_t>(sqlite3_column_int(st, 1));
  sqlite3_finalize(st);

  // same validation logic as domain::Flight::is_seat_valid
  if (seat.row() < 1 || seat.row() > rows) return false;
  const char max_letter = static_cast<char>('A' + spr - 1);
  if (!(seat.letter() >= 'A' && seat.letter() <= max_letter)) return false;

  // Atomic insert with UNIQUE(PK) constraint
  const char* sql2 =
      "INSERT OR IGNORE INTO booked_seats(flight_id, seat_row, seat_letter) VALUES(?,?,?);";
  sqlite3_stmt* st2 = nullptr;
  ok(sqlite3_prepare_v2(db_, sql2, -1, &st2, nullptr), db_, "prepare book seat");

  sqlite3_bind_int64(st2, 1, static_cast<sqlite3_int64>(flight_id.value()));
  sqlite3_bind_int(st2, 2, static_cast<int>(seat.row()));

  const std::string letter(1, seat.letter());
  sqlite3_bind_text(st2, 3, letter.c_str(), -1, SQLITE_TRANSIENT);

  ok(sqlite3_step(st2), db_, "step book seat");
  sqlite3_finalize(st2);

  return sqlite3_changes(db_) == 1;
}

void SqliteFlightRepository::release_seat(flight::domain::FlightId flight_id, const flight::domain::Seat& seat) {
  std::lock_guard<std::mutex> lock(mu_);

  const char* sql =
      "DELETE FROM booked_seats WHERE flight_id=? AND seat_row=? AND seat_letter=?;";

  sqlite3_stmt* st = nullptr;
  ok(sqlite3_prepare_v2(db_, sql, -1, &st, nullptr), db_, "prepare release seat");

  sqlite3_bind_int64(st, 1, static_cast<sqlite3_int64>(flight_id.value()));
  sqlite3_bind_int(st, 2, static_cast<int>(seat.row()));
  const std::string letter(1, seat.letter());
  sqlite3_bind_text(st, 3, letter.c_str(), -1, SQLITE_TRANSIENT);

  ok(sqlite3_step(st), db_, "step release seat");
  sqlite3_finalize(st);
}

} // namespace flight::infrastructure

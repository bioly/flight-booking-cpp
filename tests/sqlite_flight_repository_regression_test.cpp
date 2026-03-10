#include <gtest/gtest.h>

#include "flight/infrastructure/atomic_id_generator.hpp"
#include "flight/infrastructure/sqlite_flight_repository.hpp"


#include <chrono>
#include <future>

namespace {

flight::domain::Flight make_test_flight(
    flight::infrastructure::AtomicIdGenerator& ids,
    const std::string& origin,
    const std::string& destination) {
  const auto now = std::chrono::system_clock::now();

  return flight::domain::Flight(
      ids.next_flight_id(),
      flight::domain::AirportCode(origin),
      flight::domain::AirportCode(destination),
      now,
      /*rows*/ 10,
      /*seats_per_row*/ 6);
}

} // namespace

TEST(SqliteFlightRepositoryRegression, SearchDoesNotDeadlockAndReturnsMatchingFlight) {
  flight::infrastructure::AtomicIdGenerator ids;
  flight::infrastructure::SqliteFlightRepository repo(/*in_memory=*/true);

  const auto flight = make_test_flight(ids, "WAW", "FRA");
  const auto flight_id = flight.id();

  repo.upsert(flight);

  const flight::application::FlightSearchCriteria criteria{
      flight::domain::AirportCode("WAW"),
      flight::domain::AirportCode("FRA")
  };

  // Run search asynchronously so the test can detect a deadlock/hang.
  auto future = std::async(std::launch::async, [&repo, &criteria] {
    return repo.search(criteria);
  });

  const auto status = future.wait_for(std::chrono::seconds(2));
  ASSERT_EQ(status, std::future_status::ready) << "search() appears to have deadlocked";

  const auto results = future.get();
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results.front().id(), flight_id);
  EXPECT_EQ(results.front().origin(), flight::domain::AirportCode("WAW"));
  EXPECT_EQ(results.front().destination(), flight::domain::AirportCode("FRA"));
}


TEST(SqliteFlightRepositoryRegression, GetAfterBookingReturnsBookedSeatState) {
  flight::infrastructure::AtomicIdGenerator ids;
  flight::infrastructure::SqliteFlightRepository repo(/*in_memory=*/true);

  const auto flight = make_test_flight(ids, "WAW", "FRA");
  const auto flight_id = flight.id();

  repo.upsert(flight);
  ASSERT_TRUE(repo.try_book_seat(flight_id, flight::domain::Seat{1, 'A'}));

  const auto got = repo.get(flight_id);
  ASSERT_TRUE(got.has_value());
  EXPECT_TRUE(got->is_booked(flight::domain::Seat{1, 'A'}));
}

#include "flight/application/booking_service.hpp"
#include "flight/application/flight_search_service.hpp"
#include "flight/infrastructure/atomic_id_generator.hpp"
#include "flight/infrastructure/in_memory_flight_repository.hpp"
#include "flight/infrastructure/in_memory_reservation_repository.hpp"
#include "flight/infrastructure/system_clock.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace flight;

TEST(BookingConcurrency, OnlyOneThreadCanBookSameSeat) {
  infrastructure::AtomicIdGenerator ids;
  infrastructure::SystemClock clock;
  infrastructure::InMemoryFlightRepository flights;
  infrastructure::InMemoryReservationRepository reservations;

  const auto f = domain::Flight(ids.next_flight_id(), domain::AirportCode("WAW"), domain::AirportCode("FRA"),
                               std::chrono::system_clock::now(), 10, 6);
  const auto flight_id = f.id();
  flights.upsert(f);

  application::BookingService booking{flights, reservations, ids, clock};
  const auto order_id = ids.next_order_id();
  const domain::Seat seat{1, 'A'};

  std::atomic<int> success_count{0};
  constexpr int kThreads = 16;

  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int i = 0; i < kThreads; ++i) {
    threads.emplace_back([&] {
      const auto res = booking.book_seat(application::BookSeatCommand{flight_id, order_id, seat});
      if (res.success) {
        success_count.fetch_add(1, std::memory_order_relaxed);
      }
    });
  }

  for (auto& t : threads) t.join();

  EXPECT_EQ(success_count.load(), 1);
  const auto list = reservations.list_by_order(order_id);
  EXPECT_EQ(static_cast<int>(list.size()), 1);
  EXPECT_EQ(list.front().seat(), seat);
}

#include "flight/application/booking_service.hpp"
#include "flight/application/flight_search_service.hpp"
#include "flight/domain/airport_code.hpp"
#include "flight/domain/flight.hpp"
#include "flight/infrastructure/atomic_id_generator.hpp"
#include "flight/infrastructure/in_memory_flight_repository.hpp"
#include "flight/infrastructure/in_memory_reservation_repository.hpp"
#include "flight/infrastructure/system_clock.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace {

std::string format_time(std::chrono::system_clock::time_point tp) {
  const auto t = std::chrono::system_clock::to_time_t(tp);
  std::tm tm{};
#if defined(_WIN32)
  localtime_s(&tm, &t);
#else
  localtime_r(&t, &tm);
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M");
  return oss.str();
}

} // namespace

int main() {
  using namespace flight;

  infrastructure::AtomicIdGenerator ids;
  infrastructure::SystemClock clock;
  infrastructure::InMemoryFlightRepository flights;
  infrastructure::InMemoryReservationRepository reservations;

  // Seed a few flights.
  const auto now = std::chrono::system_clock::now();
  const auto f1 = domain::Flight(ids.next_flight_id(), domain::AirportCode("WAW"), domain::AirportCode("FRA"),
                                now + std::chrono::hours(6), /*rows*/ 30, /*seats_per_row*/ 6);
  const auto f2 = domain::Flight(ids.next_flight_id(), domain::AirportCode("WAW"), domain::AirportCode("FRA"),
                                now + std::chrono::hours(26), 25, 6);
  const auto f3 = domain::Flight(ids.next_flight_id(), domain::AirportCode("WAW"), domain::AirportCode("CDG"),
                                now + std::chrono::hours(8), 20, 6);
  flights.upsert(f1);
  flights.upsert(f2);
  flights.upsert(f3);

  application::FlightSearchService search{flights};
  application::BookingService booking{flights, reservations, ids, clock};

  // In a real UI/API, OrderId would come from the purchasing flow.
  const auto order_id = ids.next_order_id();

  std::cout << "Flight Booking System (v1)\n";
  std::cout << "OrderId: " << order_id << "\n\n";

  while (true) {
    std::cout << "Choose: [1] Search flights  [2] Book seat  [3] List my reservations  [0] Exit\n> ";
    int choice = 0;
    if (!(std::cin >> choice)) return 0;

    if (choice == 0) break;

    if (choice == 1) {
      std::string from, to;
      std::cout << "Origin (e.g. WAW): ";
      std::cin >> from;
      std::cout << "Destination (e.g. FRA): ";
      std::cin >> to;

      application::FlightSearchCriteria c{domain::AirportCode(from), domain::AirportCode(to)};
      const auto results = search.search(c);

      if (results.empty()) {
        std::cout << "No flights found.\n\n";
        continue;
      }

      std::cout << "Found flights:\n";
      for (const auto& f : results) {
        std::cout << "  FlightId " << f.id() << "  " << f.origin().value() << "->" << f.destination().value()
                  << "  dep: " << format_time(f.departure()) << "  cap: " << f.capacity() << "\n";
      }
      std::cout << "\n";

    } else if (choice == 2) {
      std::uint64_t flight_id_v = 0;
      std::uint16_t row = 0;
      char letter = 'A';

      std::cout << "FlightId: ";
      std::cin >> flight_id_v;
      std::cout << "Seat row: ";
      std::cin >> row;
      std::cout << "Seat letter (A-F, etc.): ";
      std::cin >> letter;

      const application::BookSeatCommand cmd{domain::FlightId{flight_id_v}, order_id, domain::Seat{row, letter}};
      const auto res = booking.book_seat(cmd);

      if (!res.success) {
        std::cout << "Booking failed: " << res.error << "\n\n";
      } else {
        std::cout << "Booked! ReservationId " << res.reservation->id() << " Seat " << res.reservation->seat().to_string()
                  << "\n\n";
      }

    } else if (choice == 3) {
      const auto list = reservations.list_by_order(order_id);
      if (list.empty()) {
        std::cout << "No reservations yet.\n\n";
        continue;
      }
      std::cout << "Reservations for OrderId " << order_id << ":\n";
      for (const auto& r : list) {
        std::cout << "  ReservationId " << r.id() << "  FlightId " << r.flight_id() << "  Seat " << r.seat().to_string()
                  << "\n";
      }
      std::cout << "\n";

    } else {
      std::cout << "Unknown option.\n\n";
    }
  }

  std::cout << "Bye.\n";
  return 0;
}

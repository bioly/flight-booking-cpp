#pragma once

#include "flight/domain/flight.hpp"
#include "flight/domain/ids.hpp"

#include <optional>
#include <vector>

namespace flight::application {

struct FlightSearchCriteria {
  flight::domain::AirportCode origin;
  flight::domain::AirportCode destination;
  // For v1 we ignore date range filtering, but keep it extensible.
};

class IFlightRepository {
public:
  virtual ~IFlightRepository() = default;

  virtual std::optional<flight::domain::Flight> get(flight::domain::FlightId id) const = 0;
  virtual std::vector<flight::domain::Flight> search(const FlightSearchCriteria& criteria) const = 0;

  // Modify operations.
  virtual void upsert(flight::domain::Flight flight) = 0;

  // Atomic seat booking operation (thread-safe):
  // - returns true if booking succeeded
  // - returns false if seat invalid or already booked
  virtual bool try_book_seat(flight::domain::FlightId flight_id, const flight::domain::Seat& seat) = 0;
  virtual void release_seat(flight::domain::FlightId flight_id, const flight::domain::Seat& seat) = 0;
};

} // namespace flight::application

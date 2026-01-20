#pragma once

#include "flight/application/flight_repository.hpp"

#include <shared_mutex>
#include <unordered_map>

namespace flight::infrastructure {

class InMemoryFlightRepository final : public flight::application::IFlightRepository {
public:
  std::optional<flight::domain::Flight> get(flight::domain::FlightId id) const override;
  std::vector<flight::domain::Flight> search(const flight::application::FlightSearchCriteria& criteria) const override;
  void upsert(flight::domain::Flight flight) override;

  bool try_book_seat(flight::domain::FlightId flight_id, const flight::domain::Seat& seat) override;
  void release_seat(flight::domain::FlightId flight_id, const flight::domain::Seat& seat) override;

private:
  // A single repository-wide shared_mutex keeps v1 simple.
  // It enables many concurrent readers (search/get) while serializing modifications (booking/upsert).
  mutable std::shared_mutex mu_;
  std::unordered_map<flight::domain::FlightId::value_type, flight::domain::Flight> flights_;
};

} // namespace flight::infrastructure

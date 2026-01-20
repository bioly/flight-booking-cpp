#include "flight/infrastructure/in_memory_flight_repository.hpp"

#include <algorithm>
#include <mutex>

namespace flight::infrastructure {

std::optional<flight::domain::Flight> InMemoryFlightRepository::get(flight::domain::FlightId id) const {
  std::shared_lock lk(mu_);
  auto it = flights_.find(id.value());
  if (it == flights_.end()) return std::nullopt;
  return it->second;
}

std::vector<flight::domain::Flight> InMemoryFlightRepository::search(
    const flight::application::FlightSearchCriteria& criteria) const {
  std::shared_lock lk(mu_);
  std::vector<flight::domain::Flight> out;
  out.reserve(flights_.size());
  for (const auto& [_, f] : flights_) {
    if (f.origin() == criteria.origin && f.destination() == criteria.destination) {
      out.push_back(f);
    }
  }
  // Sort deterministic by id
  std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.id() < b.id(); });
  return out;
}

void InMemoryFlightRepository::upsert(flight::domain::Flight flight) {
  std::unique_lock lk(mu_);
  flights_.insert_or_assign(flight.id().value(), std::move(flight));
}

bool InMemoryFlightRepository::try_book_seat(flight::domain::FlightId flight_id, const flight::domain::Seat& seat) {
  std::unique_lock lk(mu_);
  auto it = flights_.find(flight_id.value());
  if (it == flights_.end()) return false;
  auto& f = it->second;
  if (!f.is_seat_valid(seat) || f.is_booked(seat)) return false;
  // Will throw only if invalid / already booked, but we checked.
  f.book_seat(seat);
  return true;
}

void InMemoryFlightRepository::release_seat(flight::domain::FlightId flight_id, const flight::domain::Seat& seat) {
  std::unique_lock lk(mu_);
  auto it = flights_.find(flight_id.value());
  if (it == flights_.end()) return;
  it->second.release_seat(seat);
}

} // namespace flight::infrastructure

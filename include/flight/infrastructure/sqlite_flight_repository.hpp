#pragma once

#include "flight/application/flight_repository.hpp"

#include <mutex>

struct sqlite3;

namespace flight::infrastructure {

class SqliteFlightRepository final : public flight::application::IFlightRepository {
public:
  explicit SqliteFlightRepository(bool in_memory = true);
  ~SqliteFlightRepository() override;

  std::optional<flight::domain::Flight> get(flight::domain::FlightId id) const override;
  std::vector<flight::domain::Flight> search(const flight::application::FlightSearchCriteria& criteria) const override;
  void upsert(flight::domain::Flight flight) override;

  bool try_book_seat(flight::domain::FlightId flight_id, const flight::domain::Seat& seat) override;
  void release_seat(flight::domain::FlightId flight_id, const flight::domain::Seat& seat) override;

private:
  void prepare_schema();
  void exec(const char* sql) const;

  flight::domain::Flight load_flight_by_id_locked(flight::domain::FlightId id) const;

  sqlite3* db_{nullptr};
  mutable std::mutex mu_;
};

} // namespace flight::infrastructure

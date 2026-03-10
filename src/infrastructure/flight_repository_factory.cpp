#include "flight/infrastructure/flight_repository_factory.hpp"
#include "flight/infrastructure/in_memory_flight_repository.hpp"
#include "flight/infrastructure/sqlite_flight_repository.hpp"

#include <stdexcept>

namespace flight::infrastructure {

FlightRepoType parse_flight_repo_type(const std::string& value) {
  if (value == "inmem") return FlightRepoType::InMemory;
  if (value == "sqlite") return FlightRepoType::Sqlite;
  throw std::invalid_argument("Unknown --flight-repo value: " + value + " (use inmem|sqlite)");
}

std::unique_ptr<flight::application::IFlightRepository>
make_flight_repository(FlightRepoType type) {
  switch (type) {
    case FlightRepoType::InMemory:
      return std::make_unique<InMemoryFlightRepository>();
    case FlightRepoType::Sqlite:
      return std::make_unique<SqliteFlightRepository>(true); // :memory:
  }
  throw std::logic_error("Unhandled FlightRepoType");
}

} // namespace flight::infrastructure

#pragma once

#include "flight/application/flight_repository.hpp"

#include <memory>
#include <string>

namespace flight::infrastructure {

enum class FlightRepoType { InMemory, Sqlite };

FlightRepoType parse_flight_repo_type(const std::string& value);

std::unique_ptr<flight::application::IFlightRepository>
make_flight_repository(FlightRepoType type);

} // namespace flight::infrastructure

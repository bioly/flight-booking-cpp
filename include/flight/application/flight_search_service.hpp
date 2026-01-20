#pragma once

#include "flight/application/flight_repository.hpp"

namespace flight::application {

class FlightSearchService final {
public:
  explicit FlightSearchService(const IFlightRepository& flights) : flights_(flights) {}

  std::vector<flight::domain::Flight> search(FlightSearchCriteria criteria) const {
    return flights_.search(criteria);
  }

private:
  const IFlightRepository& flights_;
};

} // namespace flight::application

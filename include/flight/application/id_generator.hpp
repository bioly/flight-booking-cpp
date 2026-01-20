#pragma once

#include "flight/domain/ids.hpp"

namespace flight::application {

class IIdGenerator {
public:
  virtual ~IIdGenerator() = default;
  virtual flight::domain::FlightId next_flight_id() = 0;
  virtual flight::domain::ReservationId next_reservation_id() = 0;
  virtual flight::domain::OrderId next_order_id() = 0;
};

} // namespace flight::application

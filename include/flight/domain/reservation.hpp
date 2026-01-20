#pragma once

#include "flight/domain/ids.hpp"
#include "flight/domain/seat.hpp"

#include <chrono>
#include <string>

namespace flight::domain {

class Reservation final {
public:
  using time_point = std::chrono::system_clock::time_point;

  Reservation(ReservationId id,
              OrderId order_id,
              FlightId flight_id,
              Seat seat,
              time_point created_at)
      : id_(id), order_id_(order_id), flight_id_(flight_id), seat_(seat), created_at_(created_at) {}

  ReservationId id() const noexcept { return id_; }
  OrderId order_id() const noexcept { return order_id_; }
  FlightId flight_id() const noexcept { return flight_id_; }
  const Seat& seat() const noexcept { return seat_; }
  time_point created_at() const noexcept { return created_at_; }

private:
  ReservationId id_;
  OrderId order_id_;
  FlightId flight_id_;
  Seat seat_;
  time_point created_at_;
};

} // namespace flight::domain

#pragma once

#include "flight/application/clock.hpp"
#include "flight/application/flight_repository.hpp"
#include "flight/application/id_generator.hpp"
#include "flight/application/reservation_repository.hpp"
#include "flight/domain/reservation.hpp"

#include <optional>
#include <string>

namespace flight::application {

struct BookSeatCommand {
  flight::domain::FlightId flight_id;
  flight::domain::OrderId order_id;
  flight::domain::Seat seat;
};

struct BookSeatResult {
  bool success{false};
  std::optional<flight::domain::Reservation> reservation;
  std::string error;
};

class BookingService final {
public:
  BookingService(IFlightRepository& flights,
                 IReservationRepository& reservations,
                 IIdGenerator& ids,
                 const IClock& clock)
      : flights_(flights), reservations_(reservations), ids_(ids), clock_(clock) {}

  // Thread-safe as long as repositories are thread-safe.
  BookSeatResult book_seat(const BookSeatCommand& cmd) {
    // 1) Atomically book in flight repo (prevents double booking under contention)
    if (!flights_.try_book_seat(cmd.flight_id, cmd.seat)) {
      return BookSeatResult{false, std::nullopt, "Seat not available or invalid"};
    }

    // 2) Create reservation record.
    const auto res = flight::domain::Reservation(
        ids_.next_reservation_id(), cmd.order_id, cmd.flight_id, cmd.seat, clock_.now());

    // 3) Persist reservation.
    reservations_.add(res);

    return BookSeatResult{true, res, {}};
  }

  // Simple cancel by reservation id: releases the seat and (for v1) does not delete reservation.
  // In real systems you'd track reservation status.
  bool cancel(const flight::domain::ReservationId reservation_id) {
    const auto res = reservations_.get(reservation_id);
    if (!res) return false;
    flights_.release_seat(res->flight_id(), res->seat());
    return true;
  }

private:
  IFlightRepository& flights_;
  IReservationRepository& reservations_;
  IIdGenerator& ids_;
  const IClock& clock_;
};

} // namespace flight::application

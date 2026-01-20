# Flight Search + Seat Booking (Modern C++ v1)

A small **learning project** for modern C++ that implements:
- searching flights by origin/destination
- booking a seat for a given order
- in-memory persistence
- **thread-safe booking** for concurrent users
- unit tests with **GoogleTest**

This v1 is intentionally simple but structured so we can evolve it into a larger system (APIs, persistence, richer domain, DDD, etc.).

## Requirements
- Apple clang **14.0.0** (or any clang++ with C++20 support)
- GNU Make **3.81+**
- `git` and `cmake` (used only to fetch/build GoogleTest)

## Build
```bash
make all
```

## Run
```bash
make run
```

## Tests
```bash
make test
```

On the first test build, the Makefile will clone GoogleTest into `build/_deps/googletest/src` and build it.

## Open in VS Code / Visual Studio
This project includes `.vscode/` settings:
- **Build** task (`make all`)
- **Test** task (`make test`)
- **Debug** configuration for `bin/flight_cli`

Open the folder in VS Code and use `Run > Start Debugging`.

## Architecture & SOLID
**Modular folders**:
- `include/flight/domain` – domain entities & invariants (`Flight`, `Seat`, `Reservation`)
- `include/flight/application` – use cases (`FlightSearchService`, `BookingService`) and repository interfaces
- `include/flight/infrastructure` – in-memory repositories, system clock, atomic id generator
- `src/...` – implementations
- `tests/` – GoogleTest tests

**SOLID highlights**:
- **S**: Each class has one reason to change (e.g., booking logic vs storage)
- **O**: Repositories are interfaces; you can add new persistence without changing services
- **L**: Interfaces are substitutable (any repository implementation can replace in-memory)
- **I**: Small, focused interfaces (`IFlightRepository`, `IReservationRepository`)
- **D**: Services depend on abstractions, not concretes

## Concurrency model (v1)
- Repositories use `std::shared_mutex`:
  - `search/get` are shared (many readers)
  - `try_book_seat/upsert` are exclusive (one writer)
- `BookingService::book_seat()` relies on an **atomic seat booking operation** in the flight repository to prevent double booking under contention.

## Next steps (nice upgrades)
- Add seat maps + automatic seat selection
- Add cancellation status (don’t keep "cancel" as a side-effect only)
- Add REST API (Boost.Beast or cpp-httplib)
- Add persistent storage (SQLite)
- Add richer search filtering (date ranges, airlines, prices)

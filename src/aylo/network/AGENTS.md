# network — the transport

Moves bytes between machines: sockets, connections, acknowledgements, reliability, server and client endpoints. Static
library `network`, linked into the executable.

## Boundaries

- Knows nothing of the ECS, a plugin, a contract or any other project target. Depends on Asio and libsodium only, as
  private packages: no public header includes them; only the sources of `io/` include Asio, and only those of
  `crypto/` include libsodium.
- Namespaces mirror the folders exactly (no flat-namespace exception). A folder is created only when a file goes in it.

## Invariants

- Bytes from the network are untrusted: decoding never crashes, throws or asserts on malformed or hostile input; it
  rejects it and goes on.
- No global state; randomness and clocks are injected so tests are deterministic.
- The wire format is documented; changing it changes the protocol design document.
- Everything after the handshake is sealed (`crypto/`): a datagram that does not open, or whose counter was received
  already, is dropped like a malformed one. Keys come from the injected `RandomSource`, which must be cryptographically
  secure outside tests (`makeSystemRandom()` is).

## Tests and docs

- Tests: `tests/network/` (`network-tests`).
- Docs: `docs/network/design.md` (protocol and reasons), `docs/network/implementation.md` (API, order of the work,
  `NET-xx` tasks), `docs/network/overview.md`.

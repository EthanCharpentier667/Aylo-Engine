# Luau module error handling

How the `aylo::luau` module reports failures. The goal is one rule, so a caller can tell from a
signature whether a call can fail at run time and how.

## The rule

A failure is reported in one of two ways, depending on who caused it.

| Cause | Examples | Reported as |
| --- | --- | --- |
| The script, the data or the environment | compile or runtime error in a script, a value of the wrong type, a value that belongs to another Lua state, the Lua stack that cannot grow, out of memory, a timeout | `Result<T>` holding an `Error` |
| A bug in the calling C++ code (API misuse) | a null `lua_State*`, a moved-from handle, an operation the object does not support | an exception derived from `exceptions::Base` |

In short: **if a correct program can still hit it, it is a `Result`; if only a wrong program can hit it, it
throws.**

### `Result<T>` for recoverable failures

- The function returns `Result<T>` (`Result<void>` when there is no value) and is marked `[[nodiscard]]`.
- It never throws for these failures. Build the error with `Failure{ErrorKind::..., message}`; use
  `forwardError<T>()` to pass on the error of a sub-operation.
- Pick the `ErrorKind` that matches the cause (`kRuntime`, `kSyntax`, `kTypeMismatch`, `kStackOverflow`,
  `kOutOfMemory`...). A stack that cannot grow is `kStackOverflow`, not an exception.
- Document the failures in `@return` ("or a Failure of kind `kTypeMismatch` if...").

### Exceptions for API misuse

- The exceptions live in `Exceptions.hpp` and all derive from `exceptions::Base`:
  `InvalidState` (null state), `InvalidRef` (empty or moved-from handle), `InvalidOperation` (an operation the
  object does not support).
- They signal a programmer error. Callers are not expected to catch them, and a program that never misuses the
  API never sees one.
- Document them with `@throws`. A `@throws` of a recoverable failure (such as a stack overflow) is a sign the
  function should return a `Result` instead.

## Constructors

A constructor cannot return a `Result`, so a type whose creation can fail has a **static factory** returning
`Result<T>` (`Runtime::create`, `Table::create`, `detail::readTable`). The real constructor is private and
`noexcept` when it can be: the factory does everything that can fail first, then hands the finished parts to it.
No instance of the type is ever half built.

When a private constructor has to run code that can throw, the factory catches at its boundary and turns the
exception into a `Failure` (see `Runtime::create`).

A plain value type whose construction cannot fail (`Value`, `ValuePack`) keeps ordinary public constructors.

## The native function boundary

A native function bound to Luau (`Function`) is the one place where C++ code is called from a script. It must
not let a C++ exception unwind through Luau, so:

- a handler reports a failure by throwing an `exceptions::Base`, which becomes a Lua error with its message;
- anything else a handler throws is turned into a generic Lua error;
- a `Failure` met while reading the arguments or pushing the results becomes a Lua error too.

In the other direction, `Function::call` runs the script in protected mode and returns the script's error as a
`Failure`: of kind `kRuntime`, or one of the budget kinds below.

## Budgets: time, memory, depth

A script is game code, and a mistake in it must not take the process with it. `RuntimeConfig` gives a runtime two
limits, both off (0) by default; the engine sets them from `scripts.timeout` and `scripts.memoryLimit`.

| Failure | Limit | Kind | How |
| --- | --- | --- | --- |
| A script runs too long | `callBudget` | `kTimeout` | Luau's interrupt, called at loop back edges, calls and returns, reads the clock every 1024 calls and raises an error once the deadline has passed |
| A script holds too much | `memoryLimit` | `kOutOfMemory` | the state's allocator counts the bytes and refuses to grow past the limit, which Luau raises as a memory error |
| Calls nest too deep | Luau's own (`LUAI_MAXCALLS`, about 20000) | `kStackOverflow` | Luau's "stack overflow" error, recognized by its message |

Every call from C++ into a script goes through one of three places, which all apply them (`detail::CallScope`,
`detail::failureKind`): `Runtime::run` (and `call`, through `Function::call`), `Function::call`, and the first
`require` of a module (`ModuleLoader`).

- **The deadline belongs to the outermost call.** It starts when C++ calls into a script with no other call running;
  a script that calls C++ that calls a script again (an engine call running a callback, a `require`) shares it.
- **It is wall-clock time and cooperative.** Time spent in C++ during the call counts, but only Luau code is stopped:
  one long C++ call is not cut, and the script is stopped at its next back edge or call.
- **A script cannot outlive its timeout with `pcall`.** Once the deadline has passed, every interrupt raises again, so
  a `pcall` that caught the error is left at the next back edge.
- **The runtime stays usable.** After a timeout the next call gets a whole budget. After a memory error the runtime
  runs a full collection, since Luau does not collect when an allocation fails: what the failed script held is free
  for the next one.
- **The memory is the state's**, every script and table of the runtime together, counted from its creation (the
  standard libraries included). `Runtime::memoryInUse()` reads it.

The budget lives in `detail::Budget`, made before the state and given to `lua_newstate` as the allocator's userdata
(any thread finds it with `lua_getallocf`); the runtime destroys it after closing the state, which frees through it.

## Adding a function

1. Can a correct program see this failure (script, data, environment)? Return `Result<T>`.
2. Is it a mistake in the calling code? Throw from `exceptions/`, document it with `@throws`, and check it before
   touching the Lua stack, so the stack is left as it was.
3. Either way, leave the Lua stack as it was found when the call fails.

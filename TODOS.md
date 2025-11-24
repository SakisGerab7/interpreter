# 🛠️ Language Development TODOs

## 🧠 Core Language Features

* [x] Variable declarations (`let`)
* [x] Expression evaluation (arithmetic, string, boolean)
* [x] Assignment expressions (`a = 1`)
* [x] Scope and shadowing
* [x] Global and local variable resolution tracking
* [ ] Constants / `const` keyword support

---

## 🧾 Statements & Control Flow

* [x] `disp` statement (print)
* [x] Block statements (`{ ... }`)
* [x] `if` / `else` statements
* [x] `while` loop
* [x] C-style `for` loop
* [ ] `for-in` loop
* [ ] `break` / `continue` support
* [x] `return` statement
* [ ] Null coalescing (`a ?? b`)

---

## 🧮 Expressions

* [x] Arithmetic operations (`+`, `-`, `*`, `/`, `%`)
* [x] Logical operations (`&&`, `||`, `!`)
* [x] Equality and comparison (`==`, `!=`, `<`, `>`, `<=`, `>=`)
* [x] String concatenation (`+`)
* [x] Ternary expression (`cond ? then : else`)
* [x] Bitwise (`&`, `|`, `^`, `~`, `<<`, `>>`)
* [x] Compound assignment (`+=`, `-=`, `*=`, etc.)
* [ ] Exponentiation (`**`)
* [x] Type casting (`int()`, `float()`, `str()`)
* [ ] Type coercion and runtime type errors

---

## 🗃️ Types & Values

* [x] `int`, `float`, `bool`, `string`, `null`
* [x] Arrays/lists
* [x] Dictionaries/maps
* [x] Type introspection (`type(x)`)

### ➕ Vector & Matrix Support

* [ ] Vector literal syntax (e.g., `[1, 2, 3]`)
* [ ] Matrix literal syntax (e.g., `[[1, 2], [3, 4]]`)
* [ ] Element access (`v[0]`, `m[1][2]`)
* [ ] Vector arithmetic: `+`, `-`, scalar `*`, dot product
* [ ] Matrix operations: multiply, transpose, identity
* [ ] Built-in functions: `length(v)`, `normalize(v)`, `det(m)`

### ➕ Complex Number Support

* [ ] Complex literal syntax (e.g., `1 + 2i`)
* [ ] Complex arithmetic: `+`, `-`, `*`, `/`, conjugate
* [ ] Built-in complex functions: `abs()`, `arg()`, `real()`, `imag()`
* [ ] Compatibility with float/integer operations
* [ ] Type promotion rules

---

## 🧭 Functions

* [x] Function declarations (`function name(args) { ... }`)
* [x] Function expressions (anonymous functions, closures)
* [x] Recursion
* [x] First-class functions (pass as values)
* [x] Built-in/native functions

---

## 🧪 Standard Library / Built-ins

* [x] `disp(...)`
* [x] `clock()`, `len()`, `input()`, etc.
* [x] Math utilities (`sqrt`, `abs`, `sin`, etc.)
* [ ] String utilities (`split`, `replace`, etc.)
* [ ] Vector/matrix helpers (`dot`, `cross`, `transpose`, etc.)
* [ ] Complex helpers (`conjugate`, `magnitude`, etc.)

---

## ⚙️ Concurrency

* [x] Coroutine support (`spawn`, `yield`, `await`)
* [ ] Channels / message-passing
* [ ] Thread-safe standard library primitives
* [x] Event loop or scheduler

---

## 🌐 Networking

### 🔌 Core Networking

* [ ] Socket API (open/close, send, receive)
* [ ] TCP client
* [ ] TCP server
* [ ] UDP client/server

### 🌐 Higher-level Networking

* [ ] HTTP client

  * [ ] GET/POST requests
  * [ ] JSON parsing
* [ ] WebSocket support
* [ ] Event-based I/O (`onReceive`, `onConnect`, etc.)
* [ ] Async networking (`await socket.read()`)

---

## 🛡️ Error Handling (Rust-style)

* [ ] `Result<T, E>` type
* [ ] `Option<T>` type
* [ ] Pattern matching for `Ok`/`Err` and `Some`/`None`
* [ ] `?` operator for early return
* [ ] Error propagation across function calls
* [ ] Custom error types (`struct FileError`, etc.)
* [ ] Integration with native exceptions for FFI interop

---

## 🧰 Tools & Infrastructure

* [x] Lexer
* [x] Parser
* [x] AST nodes
* [x] Interpreter
* [x] Resolver / static analyzer (for scope resolution)
* [ ] REPL
* [ ] Test framework / unit tests for parser/interpreter
* [ ] Debugger or trace output

---

## 🧱 Advanced Features (Optional)

* [x] Structs or custom types
* [ ] Modules and imports
* [ ] Pattern matching
* [ ] Error handling (`try` / `catch`) — traditional
* [ ] Meta-programming / macros

---
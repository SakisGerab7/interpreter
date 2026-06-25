# Interpreter

A custom programming language and bytecode virtual machine implemented in C++.

The project began as an experiment in language implementation and has gradually evolved into a managed runtime featuring closures, coroutines, asynchronous I/O, runtime serialization, and garbage collection.

## Features

* Stack-based bytecode virtual machine
* Dynamic type system
* Lexical closures and captured variables
* First-class functions
* Arrays, records, structures and instances
* Green-thread scheduler
* Cooperative multitasking
* Non-blocking standard/file/socket I/O
* Channels/pipes and select-style operations
* Mark-and-sweep garbage collector
* Runtime state serialization
* VM image save/restore support

## Example

```javascript
fn make_counter() {
    let count = 0;

    fn counter() {
        return ++count;
    }

    return counter;
}

let my_counter = make_counter();

disp my_counter(); // 1
disp my_counter(); // 2
disp my_counter(); // 3
disp my_counter(); // 4
disp my_counter(); // 5
```

Example 1: The nested function captures `count` through an upvalue, allowing state to persist after the outer function returns.

```javascript
let p = pipe(0);

let producer = spawn {
    for let i = 0; i < 6; i++ {
        p <- i;
        disp "Produced: " + i;
    }
    close p;
};

let consumer = spawn {
    while p {
        let item = <-p;
        if (item != null) {
            disp "Consumed: " + item;
        }
    }
};

producer.join();
consumer.join();
```

Example 2: producer-consumer example with green threads and pipe-based message passing

## Building

```bash
cmake -S . -B build
cmake --build build
```

## Running

```bash
build/interp [source file] [args...]
```

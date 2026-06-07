import math
import time
from queue import Queue
from threading import Thread
from typing import List, Tuple


def is_prime(n: int) -> bool:
    if n < 2:
        return False
    if n == 2:
        return True
    if n % 2 == 0:
        return False
    limit = int(math.sqrt(n))
    i = 3
    while i <= limit:
        if n % i == 0:
            return False
        i += 2
    return True


def worker(tasks: Queue, results: Queue) -> None:
    total = 0
    while True:
        task = tasks.get()
        if task is None:
            break
        start, end = task
        for n in range(start, end + 1):
            if is_prime(n):
                total += n
    results.put(total)


def main() -> None:
    max_n = 256 * 1024
    chunk = 1024
    workers = 4

    tasks: Queue = Queue(maxsize=64)
    results: Queue = Queue(maxsize=64)

    start = time.perf_counter()

    threads: List[Thread] = []
    for _ in range(workers):
        th = Thread(target=worker, args=(tasks, results))
        threads.append(th)
        th.start()

    for s in range(2, max_n + 1, chunk):
        e = min(s + chunk - 1, max_n)
        tasks.put((s, e))

    for _ in range(workers):
        tasks.put(None)

    total = 0
    for _ in range(workers):
        total += results.get()

    for th in threads:
        th.join()

    elapsed = time.perf_counter() - start

    print(f"sum={total}")
    print(f"secs={elapsed:.4f} seconds")


if __name__ == "__main__":
    main()

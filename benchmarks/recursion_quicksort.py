import time


def lcg_next(x: int) -> int:
    return (x * 1103515245 + 12345) % 2_147_483_647


def quicksort(arr, left, right):
    if left >= right:
        return
    pivot = arr[(left + right) // 2]
    i, j = left, right
    while i <= j:
        while arr[i] < pivot:
            i += 1
        while arr[j] > pivot:
            j -= 1
        if i <= j:
            arr[i], arr[j] = arr[j], arr[i]
            i += 1
            j -= 1
    if left < j:
        quicksort(arr, left, j)
    if i < right:
        quicksort(arr, i, right)


def main() -> None:
    n = 256 * 1024
    seed = 1_234_567
    arr = []

    for _ in range(n):
        seed = lcg_next(seed)
        arr.append(seed % 1_000_000)

    start = time.perf_counter()
    quicksort(arr, 0, len(arr) - 1)
    elapsed = time.perf_counter() - start

    checksum = 0
    for v in arr:
        checksum = (checksum + v) % 2_147_483_647

    print(f"sum={checksum}")
    print(f"secs={elapsed:.4f} seconds")


if __name__ == "__main__":
    main()

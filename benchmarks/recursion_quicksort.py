import struct
import sys
import time


def swap(arr, i, j):
    arr[i], arr[j] = arr[j], arr[i]


def partition(arr, left, right):
    pivot = arr[int((left + right) / 2)]
    i = left
    j = right

    while i <= j:
        while arr[i] < pivot:
            i += 1

        while arr[j] > pivot:
            j -= 1

        if i <= j:
            swap(arr, i, j)
            i += 1
            j -= 1

    return i


def quicksort(arr):
    def quicksort_rec(arr, left, right):
        if left >= right:
            return

        index = partition(arr, left, right)

        if left < index - 1:
            quicksort_rec(arr, left, index - 1)

        if index < right:
            quicksort_rec(arr, index, right)

    return quicksort_rec(arr, 0, len(arr) - 1)


def read_numbers_from_binary_file(path):
    with open(path, "rb") as file:
        data = file.read()

    return list(struct.unpack(f"{len(data) // 4}i", data))


def is_sorted(arr):
    for i in range(len(arr) - 1):
        if arr[i] > arr[i + 1]:
            return False

    return True


def main():
    args = sys.argv

    if len(args) != 2:
        return 1

    arr = read_numbers_from_binary_file(f"benchmarks/numbers_{args[1]}.bin")

    start = time.perf_counter()
    quicksort(arr)
    elapsed = time.perf_counter() - start

    print(f"size   = {len(arr)}")
    print(f"sorted = {is_sorted(arr)}")
    print(f"secs   = {elapsed:.6f}")


if __name__ == "__main__":
    sys.exit(main())

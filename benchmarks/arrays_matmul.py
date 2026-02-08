import time
import random

def main() -> None:
    n = 180

    a = []
    b = []
    c = []

    random.seed(0)    

    for i in range(n):
        row_a = []
        row_b = []
        row_c = []
        for j in range(n):
            row_a.append(random.randint(0, 100))
            row_b.append(random.randint(0, 100))
            row_c.append(0)

        a.append(row_a)
        b.append(row_b)
        c.append(row_c)

    start = time.perf_counter()
    for i in range(n):
        row_a = a[i]
        for k in range(n):
            aik = row_a[k]
            row_b = b[k]
            row_c = c[i]
            for j in range(n):
                row_c[j] += aik * row_b[j]
    elapsed = time.perf_counter() - start

    print(f"Elapsed time: {elapsed:.4f} seconds")


if __name__ == "__main__":
    main()

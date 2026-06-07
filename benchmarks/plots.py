results = {
    "Python": {
        "matmul": {
            16: 0.0004,
            32: 0.0028,
            64: 0.0215,
            128: 0.1697,
            256: 1.3655,
        },
        "quicksort": {
            16*1024: 0.0204,
            32*1024: 0.0435,
            64*1024: 0.0963,
            128*1024: 0.2083,
            256*1024: 0.4670,
        },
        "nbody": {
            16: 0.0012,
            32: 0.0048,
            64: 0.0185,
            128: 0.0731,
            256: 0.2937,
        },
        "primes": {
            16*1024: 0.0095,
            32*1024: 0.0248,
            64*1024: 0.0679,
            128*1024: 0.1704,
            256*1024: 0.6401,
        },
    },
    "My Language": {
        "matmul": {
            16: 0.003,
            32: 0.025,
            64: 0.199,
            128: 1.563,
            256: 12.256,
        },
        "quicksort": {
            16*1024: 0.224,
            32*1024: 0.49,
            64*1024: 1.023,
            128*1024: 2.197,
            256*1024: 4.372,
        },
        "nbody": {
            16: 0.011,
            32: 0.042,
            64: 0.17,
            128: 0.669,
            256: 2.67,
        },
        "primes": {
            16*1024: 0.066,
            32*1024: 0.163,
            64*1024: 0.397,
            128*1024: 0.983,
            256*1024: 2.451,
        },
    }
}

import matplotlib.pyplot as plt
import numpy as np

fig, axs = plt.subplots(2, 2, figsize=(12, 8))
benchmarks = ["matmul", "quicksort", "nbody", "primes"]
for i, benchmark in enumerate(benchmarks):
    ax = axs[i // 2, i % 2]
    for impl in results:
        sizes = sorted(results[impl][benchmark].keys())
        times = [results[impl][benchmark][size] for size in sizes]
        ax.plot(sizes, times, marker='o', label=impl)
    ax.set_xscale('log')
    ax.set_yscale('log')
    avg_ratio = np.mean([results["My Language"][benchmark][size] / results["Python"][benchmark][size] for size in sizes])
    ax.set_title(f"{benchmark} ({avg_ratio:.1f}x slower than Python)")
    ax.set_xlabel('Input Size')
    ax.set_ylabel('Time (s)')
    ax.set_xticks(sizes, labels=[str(size) for size in sizes])
    ax.legend()
plt.tight_layout()
plt.savefig('benchmark_results.png')
plt.show()
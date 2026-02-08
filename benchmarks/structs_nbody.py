import time
import math
from dataclasses import dataclass


@dataclass
class Body:
    x: float
    y: float
    z: float
    vx: float
    vy: float
    vz: float
    m: float


def lcg_next(x: int) -> int:
    return (x * 1103515245 + 12345) % 2_147_483_647


def main() -> None:
    n = 200
    steps = 10
    dt = 0.01
    seed = 7

    bodies = []
    for _ in range(n):
        seed = lcg_next(seed)
        x = (seed % 1000) / 100.0
        seed = lcg_next(seed)
        y = (seed % 1000) / 100.0
        seed = lcg_next(seed)
        z = (seed % 1000) / 100.0
        seed = lcg_next(seed)
        vx = (seed % 1000) / 10000.0
        seed = lcg_next(seed)
        vy = (seed % 1000) / 10000.0
        seed = lcg_next(seed)
        vz = (seed % 1000) / 10000.0
        seed = lcg_next(seed)
        m = (seed % 1000) / 100.0 + 1.0
        bodies.append(Body(x, y, z, vx, vy, vz, m))

    start = time.perf_counter()
    for _ in range(steps):
        for i in range(n):
            bi = bodies[i]
            ax = ay = az = 0.0
            for j in range(n):
                if i == j:
                    continue
                bj = bodies[j]
                dx = bj.x - bi.x
                dy = bj.y - bi.y
                dz = bj.z - bi.z
                dist2 = dx * dx + dy * dy + dz * dz + 0.01
                inv = 1.0 / (dist2 * math.sqrt(dist2))
                s = bj.m * inv
                ax += dx * s
                ay += dy * s
                az += dz * s
            bi.vx += ax * dt
            bi.vy += ay * dt
            bi.vz += az * dt

        for i in range(n):
            b = bodies[i]
            b.x += b.vx * dt
            b.y += b.vy * dt
            b.z += b.vz * dt

    elapsed = time.perf_counter() - start

    checksum = 0.0
    for b in bodies:
        checksum += b.x + b.y + b.z + b.vx + b.vy + b.vz

    print(f"sum={checksum}")
    print(f"secs={elapsed}")


if __name__ == "__main__":
    main()

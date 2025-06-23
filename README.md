# 💧 Building H₂O

This project simulates the process of building water molecules (H₂O) using parallel processes and synchronization techniques. It is designed for an operating systems course at **FIT VUT Brno**.

## Author

- **Martin Valach** – `xvalac12@stud.fit.vutbr.cz`

## Project Description

The goal of the project is to simulate the chemical process of creating water molecules using the correct ratio: **2 Hydrogen atoms (H)** and **1 Oxygen atom (O)**. The simulation uses:

- POSIX processes (`fork()`)
- POSIX semaphores (`sem_open`, `sem_wait`, `sem_post`)
- Shared memory (`shm_open`, `mmap`)
- Synchronization mechanisms to ensure that exactly two H and one O are used for each H₂O molecule.

Each process (Hydrogen or Oxygen) prints its state to a shared output file `proj2.out`.

## Compilation

To compile the project, use:

```bash
gcc -Wall -Wextra -Werror -std=gnu11 proj2.c -o proj2 -pthread
```

## 🔧 Usage

```bash
./proj2 [NO] [NH] [TI] [TB]
```

---

### 📑 Filters

| Parameter | Description|
|----------|-------------|
| `NO` | Number of oxygen atoms to generate |
| `NH` | Number of hydrogen atoms to generate |
| `TI` | Maximum time in milliseconds (0 ≤ TI ≤ 1000) for an atom to wait before joining the queue |
| `TB` | Maximum time in milliseconds (0 ≤ TB ≤ 1000) for bonding (reaction) time |

## 💡 Example:

```bash
./proj2 3 6 100 100
```

## Output

The program writes output to `proj2.out`, containing logs of all atomic events:
- Atom creation
- Joining the reaction queue
- Molecule creation and completion
- Fallbacks when atoms cannot participate

## Synchronization Overview

The following synchronization techniques are used:

- **Semaphores**: Control the flow of processes to ensure proper bonding (2 H + 1 O).
- **Shared memory**: Used for counters and flags shared between processes.
- **Process coordination**: Ensures that atoms that cannot participate are properly terminated and logged.

## Error Handling

The program checks:
- Argument count and values
- Semaphore and shared memory creation failures
- Process forking errors

On error, it prints messages to `stderr` and cleans up all allocated resources.

## 📎 Notes

- Output is unbuffered for consistency using `setbuf(output_file, NULL)`.
- The solution is deterministic only if random delays (`TI` and `TB`) are set to zero.

## 📘 License

This project is intended for academic purposes only.

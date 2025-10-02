# Operating Systems – Assignment 3: Memory Management

This repository contains my solution for **Assignment 3** of the Operating Systems course.  
It is based on **xv6-riscv** and extends the kernel with support for shared memory and inter-process communication.

---

## Implemented Features
- **Shared Memory System Calls** – implemented `map_shared_pages` and `unmap_shared_pages` in the kernel, along with user-facing syscalls `sys_map_shared_pages` and `sys_unmap_shared_pages`.  
- **PTE_S Flag** – introduced a new page table entry flag to safely mark shared mappings.  
- **Memory Safety** – modified `uvmunmap` to prevent double freeing of shared pages.  
- **shmem_test** – user program demonstrating shared memory between parent and child processes.  
- **Multi-process Logging** – built a logging system where multiple child processes write to a shared buffer (`log_test.c`). Used atomic operations to prevent race conditions and ensure 4-byte alignment of messages.  

---

## Skills Gained
- Extending xv6 with **shared memory support**.  
- Working with **page tables** and virtual memory management.  
- Using **atomic operations** for safe synchronization.  
- Implementing a simple **IPC mechanism** using shared memory.  
- Debugging and testing multi-process memory interactions.  

---

## ▶️ How to Run
Build xv6:
```bash
make qemu

To run shared memory test:
shmem_test

To run logging system test:
log_test

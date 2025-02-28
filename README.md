## NetHash
NetHash is a proof-of-work (PoW) algorithm that utilizes network queries to ensure algorithm runtimes remain unaffected by more powerful processors, maintaining consistent runtimes across different types of processors.

### Overview
Most hash functions and proof-of-work (PoW) algorithms rely on extensive memory usage, random instructions, or CPU-intensive workloads to ensure they are ASIC-proof and GPU-proof. However, there is nothing to prevent users from purchasing more powerful processors to execute these algorithms faster. This creates an imbalance in the mining ecosystem, as the majority of computational power becomes concentrated in the hands of a few miners.

Input/output (I/O) is one of the primary bottlenecks for processors. NetHash addresses this issue by utilizing network queries to render CPU power redundant. This approach enables miners to achieve similar hashing speeds regardless of whether they are using a high-performance processor, a smartphone, or a Raspberry Pi board.

### Design
NetHash leverages the IPFS network to query a fixed set of Content Identifiers (CIDs). CIDs are unique hashes that point to data, ensuring the content is immutable and permanent. Currently, the algorithm utilizes distributed Wikipedia CIDs in various languages.

The process begins with NetHash generating a SHA-256 hash from the input data, resulting in a 32-byte output. This hash is then divided into sixteen 2-byte chunks, which are processed in a loop. Even-indexed chunks determine the link to visit, while odd-indexed chunks select 2-byte data from the visited page. These combined 16 bytes are used to generate an 8-byte hash, which is prepended to the original SHA-256 hash. The final output is a 40-byte hash.

This methodology guarantees unique final hashes. The lower 32 bytes originate from the SHA-256 algorithm, while the upper 8 bytes are derived from a fixed set of data fetched through IPFS network queries.

The added I/O overhead and the inherent unpredictability of network operations ensure that CPU power does not influence the hash rate. However, network calls may fail due to timeouts or other errors. In such cases, the algorithm returns -1 to the caller, requiring proper error handling.

### Build and Usage
NetHash generates a static C library called `libnethash.a`. A sample file demonstrating its usage can be found in [example/main.c](example/main.c).

Additionally, you can build the `nethash-tests` executable to run tests and benchmark the performance of NetHash on your system.

#### Linux
Dependencies:
```
libcurl4-openssl-dev libssl-dev libxml2-dev
```

To build `libnethash.a`, run:
```
make
``` 

To build `nethash-tests`, run:
```
make nethash-tests
```

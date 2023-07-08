# IPC
Benchmark of shared memory and socket IPC mechanisms on inter-process SPSC  

### Send 1,000,000 messages

|  socket IPC           |  shared memory IPC    |
| --------              | -------               |
| started at 2617430 us | started at 2542178 us |
| finished at 6944959 us| finished at 4381689 us|
| real    4.329s        | real    1.842s        |
| user    0.394s        | user    1.647s        |
| sys     3.861s        | sys     0.124s        |

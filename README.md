# Memory_Manager
Implement three page replacement policies

## Overview 
![](https://i.imgur.com/jW77cnL.png)

- Use an one-level page table for mapping virtual pages to
physical frames
-  Implement three page replacement policies
    • FIFO
    • Enhanced second chance algorithm
    • Segmented LRU


## Assumptions

- No TLB support
    - Each page access needs to consult the page table
- Only a single process
- Evicted page should write back to the disk whether it is
dirty page or not, which is not a general method in real
world
- Disk always has enough space for evicted pages
- There will be M virtual pages and N physical frames
    - M and N will be given in the trace file
    - M > N


## How to build
- `make` 


## Execute Command Format 
- (using I/O Redirection)
    - ./memory_manager < INPUT_FILE > OUTPUT_FILE

## Output File Format

- Show Miss/Hit and related information for each reference
    - Format for a hit: Hit, VPI=>PFI
    - Format for a miss: Miss, PFI, Evicted VPI>>Destination, VPI<<Source
        - Source: the block index of the page which is page-in from disk
        - Destination: the block index where the evicted page page-out
        - For first reference, there might be no source || destination || Evicted VPI,
set the value as -1



## Correctness

- pass all testbench from `OSLab`


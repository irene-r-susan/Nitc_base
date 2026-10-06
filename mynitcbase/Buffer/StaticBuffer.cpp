#include "StaticBuffer.h"
// the declarations for this class can be found at "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];

StaticBuffer::StaticBuffer() {

  // initialise all blocks as free
  for (int bufferIndex = 0;bufferIndex<=BUFFER_CAPACITY-1;bufferIndex++) {
    metainfo[bufferIndex].free = true;
    metainfo[bufferIndex].dirty=false;
    metainfo[bufferIndex].timeStamp=-1;
    metainfo[bufferIndex].blockNum=-1;
  }
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer() {

for (int bufferIndex = 0;bufferIndex<=BUFFER_CAPACITY-1;bufferIndex++) {
    if(metainfo[bufferIndex].free == false && metainfo[bufferIndex].dirty==true)
    {
      Disk::writeBlock(blocks[bufferIndex],metainfo[bufferIndex].blockNum);
    }
  }

}


/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum) {
  // Check if blockNum is valid (between zero and DISK_BLOCKS)
  // and return E_OUTOFBOUND if not valid.
    if(blockNum<0||blockNum>DISK_BLOCKS)
    return E_OUTOFBOUND;
  // find and return the bufferIndex which corresponds to blockNum (check metainfo)
    for(int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++)
    {
        if(!metainfo[bufferIndex].free&&metainfo[bufferIndex].blockNum==blockNum)
        {
            return bufferIndex;
        }
    }
  // if block is not in the buffer
  return E_BLOCKNOTINBUFFER;
}



int StaticBuffer::getFreeBuffer(int blockNum) {
    // 1. Check if blockNum is valid
    if (blockNum < 0 || blockNum >= DISK_BLOCKS) {
        return E_OUTOFBOUND;
    }

    // 2. Increase timeStamp of all occupied buffers
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        if (!metainfo[bufferIndex].free) {
            metainfo[bufferIndex].timeStamp++;
        }
    }

    int allocatedBuffer = -1;

    // 3. Find any buffer marked free
    for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
        if (metainfo[bufferIndex].free) {
            allocatedBuffer = bufferIndex;
            break;
        }
    }

    // 4. If no free buffer -> evict the one with the highest timeStamp (LRU)
    if (allocatedBuffer == -1) {
        int maxTimeStamp = -1;
        for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
            if (metainfo[bufferIndex].timeStamp > maxTimeStamp) {
                maxTimeStamp = metainfo[bufferIndex].timeStamp;
                allocatedBuffer = bufferIndex;
            }
        }

        // If the evicted block is dirty, write it back to disk
        if (metainfo[allocatedBuffer].dirty) {
            Disk::writeBlock(blocks[allocatedBuffer], metainfo[allocatedBuffer].blockNum);
        }
    }

    // 5. Update metadata for the allocated buffer frame
    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].dirty = false;
    metainfo[allocatedBuffer].blockNum = blockNum;
    metainfo[allocatedBuffer].timeStamp = 0;

    return allocatedBuffer;
}

int StaticBuffer::setDirtyBit(int blockNum) {
    // 1. Find the buffer index corresponding to the given block number
    int bufferNum = StaticBuffer::getBufferNum(blockNum);

    // 2. If the block is not present in the buffer pool
    if (bufferNum == E_BLOCKNOTINBUFFER) {
        return E_BLOCKNOTINBUFFER;
    }

    // 3. If the block number is out of bounds
    if (bufferNum == E_OUTOFBOUND) {
        return E_OUTOFBOUND;
    }

    // 4. Mark the buffer entry as modified (dirty)
    metainfo[bufferNum].dirty = true;

    return SUCCESS;
}
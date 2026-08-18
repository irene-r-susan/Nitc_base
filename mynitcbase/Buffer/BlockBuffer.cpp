#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

#include "BlockBuffer.h"
#include <cstring>

// Initialize blockNum member variable
BlockBuffer::BlockBuffer(int blockNum) {
  this->blockNum = blockNum;
}

// Calls the parent class constructor
RecBuffer::RecBuffer(int blockNum) : BlockBuffer(blockNum) {}



// Load the block header into the argument pointer
int BlockBuffer::getHeader(struct HeadInfo *head) {

  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;   // return any errors that might have occured in the process
  }

  unsigned char buffer[BLOCK_SIZE];

  // Read the block at this.blockNum into the buffer
  Disk::readBlock(buffer, this->blockNum);

  // Populate the fields in *head according to header layout offsets
  memcpy(&head->numSlots, buffer + 24, 4);
  memcpy(&head->numEntries, buffer + 16, 4);
  memcpy(&head->numAttrs, buffer + 20, 4);
  memcpy(&head->rblock, buffer + 12, 4);
  memcpy(&head->lblock, buffer + 8, 4);

  return SUCCESS;
}

// Load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
  struct HeadInfo head;
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  // Get the header using this->getHeader() function
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  // Read the block at this.blockNum into a buffer
  unsigned char buffer[BLOCK_SIZE];
  Disk::readBlock(buffer, this->blockNum);

  /* record at slotNum will be at offset HEADER_SIZE + slotMapSize + (recordSize * slotNum)
     - each record will have size attrCount * ATTR_SIZE
     - slotMap will be of size slotCount (1 byte per slot)
  */
  int recordSize = attrCount * ATTR_SIZE;
  int slotMapSize = slotCount;

  unsigned char *slotPointer = buffer + HEADER_SIZE + slotMapSize + (recordSize * slotNum);

  // Load the record into the rec data structure
  memcpy(rec, slotPointer, recordSize);

  return SUCCESS;
}




// write the record from the argument pointer into slotNum of this block
int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
  struct HeadInfo head;

  // 1. Fetch header info to calculate record size and total slots
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  // Validation: ensure slotNum is within valid bounds
  if (slotNum < 0 || slotNum >= slotCount) {
    return E_OUTOFBOUND;
  }

  // 2. Read the full block from disk into a local buffer
  unsigned char buffer[BLOCK_SIZE];
  Disk::readBlock(buffer, this->blockNum);

  // 3. Calculate offset: HEADER_SIZE + slotMapSize + (recordSize * slotNum)
  int recordSize = attrCount * ATTR_SIZE;
  int slotMapSize = slotCount;

  unsigned char *slotPointer = buffer + HEADER_SIZE + slotMapSize + (recordSize * slotNum);

  // 4. Copy updated record bytes into buffer
  memcpy(slotPointer, rec, recordSize);

  // 5. Write the updated block back to disk
  Disk::writeBlock(buffer, this->blockNum);

  return SUCCESS;
}

/*
Used to load a block to the buffer and get a pointer to it.
NOTE: this function expects the caller to allocate memory for the argument
*/


int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
  // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum == E_BLOCKNOTINBUFFER) {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND) {
      return E_OUTOFBOUND;
    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }

  // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
  *buffPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}



#include "BlockBuffer.h"
#include "../Disk_Class/Disk.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>

int compareAttrs(union Attribute attr1,union Attribute attr2,int attrType)
{
    double diff;
    if(attrType==STRING)
        diff = strcmp(attr1.sVal,attr2.sVal);
    else
        diff = attr1.nVal - attr2.nVal;
    if(diff>0)
        return 1;
    if(diff<0)
        return -1;
    if(diff==0)
        return 0;
    return -1;
}

BlockBuffer::BlockBuffer(char blockType) {
    int blockTypeInt;
    if (blockType == 'R') {
        blockTypeInt = REC;
    } else if (blockType == 'I') {
        blockTypeInt = IND_INTERNAL;
    } else if (blockType == 'L') {
        blockTypeInt = IND_LEAF;
    } else {
        blockTypeInt = UNUSED_BLK;
    }
    int allocatedBlock = this->getFreeBlock(blockTypeInt);

    this->blockNum = allocatedBlock;
}

RecBuffer::RecBuffer() : BlockBuffer::BlockBuffer('R') {}

BlockBuffer::BlockBuffer(int blockNum)
{
    this->blockNum = blockNum;
}

RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}

int RecBuffer::getSlotMap(unsigned char *slotmap)
{
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret!=SUCCESS)
        return ret;
    struct HeadInfo head;
    ret = getHeader(&head);
    if(ret!=SUCCESS)
        return ret;
    int slotCount = head.numSlots;

    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
    memcpy(slotmap,slotMapInBuffer,slotCount);
    return SUCCESS;
}

int RecBuffer::setSlotMap(unsigned char *slotMap) {
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
        return ret;
    }

    struct HeadInfo head;
    this->getHeader(&head);

    int numSlots = head.numSlots;

    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;
    memcpy(slotMapInBuffer, slotMap, numSlots);

    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if (ret != SUCCESS) {
        return ret;
    }

    return SUCCESS;
}

int BlockBuffer::getHeader(struct HeadInfo *head)
{
    unsigned char* bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret!=SUCCESS)
        return ret;
    memcpy(&head->numSlots,bufferPtr+24,4);
    memcpy(&head->numEntries,bufferPtr+16,4);
    memcpy(&head->numAttrs,bufferPtr+20,4);
    memcpy(&head->rblock,bufferPtr+12,4);
    memcpy(&head->lblock,bufferPtr+8,4);
    return SUCCESS;
}

int RecBuffer::getRecord(union Attribute *rec, int slotNum)
{
    struct HeadInfo head;
    this->getHeader(&head);
    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
        return ret;
    }

    int recordSize = attrCount*ATTR_SIZE;
    unsigned char *slotPointer = bufferPtr+(HEADER_SIZE + slotCount + (recordSize*slotNum));

    memcpy(rec,slotPointer,recordSize);
    return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if (ret != SUCCESS) {
        return ret;
    }

    struct HeadInfo head;
    this->getHeader(&head);

    int numAttrs = head.numAttrs;
    int numSlots = head.numSlots;

    if (slotNum < 0 || slotNum >= numSlots) {
        return E_OUTOFBOUND;
    }

    int recordSize = numAttrs * ATTR_SIZE;

    unsigned char *slotPointer = bufferPtr + HEADER_SIZE + numSlots + (slotNum * recordSize);

    memcpy(slotPointer, rec, recordSize);

    StaticBuffer::setDirtyBit(this->blockNum);

    return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr)
{
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
    if (bufferNum != E_BLOCKNOTINBUFFER)
    {
        for (int i = 0; i < BUFFER_CAPACITY;i++) {
            if (!StaticBuffer::metainfo[i].free)
                StaticBuffer::metainfo[i].timeStamp++;
        }
        StaticBuffer::metainfo[bufferNum].timeStamp = 0;
    }
    else
    {
        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        if (bufferNum == E_OUTOFBOUND) {
            return E_OUTOFBOUND;
        }

        Disk::readBlock(StaticBuffer::blocks[bufferNum],this->blockNum);
    }

    *buffPtr = StaticBuffer::blocks[bufferNum];

    return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head) {
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if (ret != SUCCESS) {
        return ret;
    }

    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

    bufferHeader->blockType = head->blockType;
    bufferHeader->pblock = head->pblock;
    bufferHeader->lblock = head->lblock;
    bufferHeader->rblock = head->rblock;
    bufferHeader->numEntries = head->numEntries;
    bufferHeader->numAttrs = head->numAttrs;
    bufferHeader->numSlots = head->numSlots;

    int retDirty = StaticBuffer::setDirtyBit(this->blockNum);

    if (retDirty != SUCCESS) {
        return retDirty;
    }

    return SUCCESS;
}
int BlockBuffer::setBlockType(int blockType) {
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);

    if (ret != SUCCESS) {
        return ret;
    }

    *((int32_t *)bufferPtr) = blockType;

    StaticBuffer::blockAllocMap[this->blockNum] = blockType;

    int retDirty = StaticBuffer::setDirtyBit(this->blockNum);

    if (retDirty != SUCCESS) {
        return retDirty;
    }

    return SUCCESS;
}
int BlockBuffer::getFreeBlock(int blockType) {
    int freeBlock = -1;

    for (int i = 0; i < DISK_BLOCKS; ++i) {
        if (StaticBuffer::blockAllocMap[i] == UNUSED_BLK) {
            freeBlock = i;
            break;
        }
    }

    if (freeBlock == -1) {
        return E_DISKFULL;
    }

    this->blockNum = freeBlock;

    int bufferNum = StaticBuffer::getFreeBuffer(freeBlock);

    if (bufferNum == E_OUTOFBOUND) {
        return E_OUTOFBOUND;
    }
    
    struct HeadInfo head;

    head.blockType = blockType;
    head.pblock = -1;
    head.lblock = -1;
    head.rblock = -1;
    head.numEntries = 0;
    head.numAttrs = 0;
    head.numSlots = 0;

    int ret = this->setHeader(&head);

    if (ret != SUCCESS) {
        return ret;
    }

    ret = this->setBlockType(blockType);

    if (ret != SUCCESS) {
        return ret;
    }

    return freeBlock;
}

void BlockBuffer::releaseBlock() {
    if (this->blockNum == INVALID_BLOCKNUM || this->blockNum < 0 || this->blockNum >= DISK_BLOCKS)
        return;
        
    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if (bufferNum != E_BLOCKNOTINBUFFER) {
        StaticBuffer::metainfo[bufferNum].free = true;
    }

    StaticBuffer::blockAllocMap[this->blockNum] = UNUSED_BLK;

    this->blockNum = INVALID_BLOCKNUM;
}

int BlockBuffer::getBlockNum()
{
    return this->blockNum;
}
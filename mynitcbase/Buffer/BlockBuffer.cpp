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
}

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

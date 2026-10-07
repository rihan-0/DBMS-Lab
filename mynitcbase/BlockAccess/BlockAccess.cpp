#include "BlockAccess.h"

#include <cstring>

RecId BlockAccess::linearSearch(int relId,char attrName[ATTR_SIZE],union Attribute attrVal, int op)
{
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId,&prevRecId);
    int block,slot;
    if(prevRecId.block == -1 && prevRecId.slot == -1)
    {
        RelCatEntry relBuf;
        RelCacheTable::getRelCatEntry(relId,&relBuf);

        block = relBuf.firstBlk;
        slot = 0;
    }
    else
    {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    while(block != -1)
    {
        RecBuffer recBuffer(block);
        
        HeadInfo relHeader;
        recBuffer.getHeader(&relHeader);
        
        unsigned char slotmap[relHeader.numSlots];
        recBuffer.getSlotMap(slotmap);

        if(slot >= relHeader.numSlots)
        {
            block = relHeader.rblock;
            slot = 0;
            continue;
        }
        if(slotmap[slot] == SLOT_UNOCCUPIED)
        {
            slot++;
            continue;
        }

        union Attribute record[relHeader.numAttrs];
        recBuffer.getRecord(record,slot);

        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId,attrName,&attrCatEntry);

        union Attribute recAttrVal  = record[attrCatEntry.offset];

        int cmpVal;
        cmpVal = compareAttrs(recAttrVal,attrVal,attrCatEntry.attrType);

        if(
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            RecId matchedRecId;
            matchedRecId.block = block;
            matchedRecId.slot = slot;
            RelCacheTable::setSearchIndex(relId,&matchedRecId);

            return matchedRecId;
        }
        slot++;
    }
    RecId retId;
    retId.block = -1;
    retId.slot = -1;
    return retId;
}


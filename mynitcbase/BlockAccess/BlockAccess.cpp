#include "BlockAccess.h"

#include <cstring>

int BlockAccess::project(int relId, union Attribute *record) {
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block, slot;

    if (prevRecId.block == -1 && prevRecId.slot == -1) {
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);

        block = relCatEntry.firstBlk;
        slot = 0;
    } else {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    while (block != -1) {
        RecBuffer recBlock(block);
        HeadInfo head;
        recBlock.getHeader(&head);

        unsigned char slotMap[head.numSlots];
        recBlock.getSlotMap(slotMap);

        if (slot >= head.numSlots) {
            block = head.rblock;
            slot = 0;
        } else if (slotMap[slot] == SLOT_UNOCCUPIED) {
            slot++;
        } else {
            break;
        }
    }

    if (block == -1) {
        return E_NOTFOUND;
    }

    RecId nextRecId{block, slot};
    RelCacheTable::setSearchIndex(relId, &nextRecId);

    RecBuffer recBlock(nextRecId.block);
    recBlock.getRecord(record, nextRecId.slot);

    return SUCCESS;
}

int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    RecId recId;

    recId = BlockAccess::linearSearch(relId, attrName, attrVal, op);

    if (recId.block == -1 && recId.slot == -1) {
        return E_NOTFOUND;
    }

    RecBuffer recBuffer(recId.block);

    recBuffer.getRecord(record, recId.slot);

    return SUCCESS;
}

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

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;
    strcpy(newRelationName.sVal, newName);

    RecId targetRecId = BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME, newRelationName, EQ);

    if (targetRecId.block != -1 && targetRecId.slot != -1) {
        return E_RELEXIST;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;
    strcpy(oldRelationName.sVal, oldName);

    RecId oldRecId = BlockAccess::linearSearch(RELCAT_RELID, (char*)RELCAT_ATTR_RELNAME, oldRelationName, EQ);

    if (oldRecId.block == -1 && oldRecId.slot == -1) {
        return E_RELNOTEXIST;
    }

    RecBuffer relCatBlock(oldRecId.block);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, oldRecId.slot);

    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, newName);
    relCatBlock.setRecord(relCatRecord, oldRecId.slot);

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numAttrs = (int)relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    for (int i = 0; i < numAttrs; i++) {
        RecId attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char*)ATTRCAT_ATTR_RELNAME, oldRelationName, EQ);

        RecBuffer attrCatBlock(attrCatRecId.block);
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, attrCatRecId.slot);

        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, newName);
        attrCatBlock.setRecord(attrCatRecord, attrCatRecId.slot);
    }

    return SUCCESS;
}
int BlockAccess::renameAttribute(char relName[ATTR_SIZE],char oldName[ATTR_SIZE],char newName[ATTR_SIZE]) {
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    RecId relRecId = BlockAccess::linearSearch(RELCAT_RELID,(char *)RELCAT_ATTR_RELNAME,relNameAttr,EQ);

    if (relRecId.block == -1 && relRecId.slot == -1)
        return E_RELNOTEXIST;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    while (true) {
        RecId attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID,(char *)ATTRCAT_ATTR_RELNAME,relNameAttr,EQ);

        if (attrCatRecId.block == -1 && attrCatRecId.slot == -1)
            break;

        RecBuffer attrCatBlock(attrCatRecId.block);
        attrCatBlock.getRecord(attrCatEntryRecord, attrCatRecId.slot);

        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,oldName) == 0) {
            attrToRenameRecId = attrCatRecId;
        }

        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName) == 0) {
            return E_ATTREXIST;
        }
    }

    if (attrToRenameRecId.block == -1 &&attrToRenameRecId.slot == -1) {
        return E_ATTRNOTEXIST;
    }

    RecBuffer attrCatBlock(attrToRenameRecId.block);
    attrCatBlock.getRecord(attrCatEntryRecord,attrToRenameRecId.slot);

    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,newName);

    attrCatBlock.setRecord(attrCatEntryRecord,attrToRenameRecId.slot);

    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record) {
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int blockNum = relCatEntry.firstBlk;
    RecId rec_id = {-1, -1};
    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;
    int prevBlockNum = -1;

    while (blockNum != -1) {
        RecBuffer blockBuffer(blockNum);
        HeadInfo head;
        blockBuffer.getHeader(&head);

        unsigned char slotMap[head.numSlots];
        blockBuffer.getSlotMap(slotMap);

        for (int slot = 0; slot < head.numSlots; slot++) {
            if (slotMap[slot] == SLOT_UNOCCUPIED) {
                rec_id.block = blockNum;
                rec_id.slot = slot;
                break;
            }
        }

        if (rec_id.block != -1 && rec_id.slot != -1) {
            break;
        }

        prevBlockNum = blockNum;
        blockNum = head.rblock;
    }

    if (rec_id.block == -1 && rec_id.slot == -1) {
        if (relId == RELCAT_RELID) {
            return E_MAXRELATIONS;
        }

        RecBuffer newBlockBuffer;
        int ret = newBlockBuffer.getBlockNum();

        if (ret == E_DISKFULL) {
            return E_DISKFULL;
        }

        rec_id.block = ret;
        rec_id.slot = 0;

        HeadInfo head;
        head.blockType = REC;
        head.pblock = -1;
        head.lblock = prevBlockNum;
        head.rblock = -1;
        head.numEntries = 0;
        head.numSlots = numOfSlots;
        head.numAttrs = numOfAttributes;

        newBlockBuffer.setHeader(&head);

        unsigned char slotMap[numOfSlots];
        for (int slot = 0; slot < numOfSlots; slot++) {
            slotMap[slot] = SLOT_UNOCCUPIED;
        }

        newBlockBuffer.setSlotMap(slotMap);

        if (prevBlockNum != -1) {
            RecBuffer prevBlockBuffer(prevBlockNum);
            HeadInfo prevHead;
            prevBlockBuffer.getHeader(&prevHead);

            prevHead.rblock = rec_id.block;
            prevBlockBuffer.setHeader(&prevHead);
        } else {
            relCatEntry.firstBlk = rec_id.block;
        }

        relCatEntry.lastBlk = rec_id.block;
        RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }

    RecBuffer recBuffer(rec_id.block);
    recBuffer.setRecord(record, rec_id.slot);

    unsigned char slotMap[numOfSlots];
    recBuffer.getSlotMap(slotMap);

    slotMap[rec_id.slot] = SLOT_OCCUPIED;
    recBuffer.setSlotMap(slotMap);

    HeadInfo head;
    recBuffer.getHeader(&head);

    head.numEntries++;
    recBuffer.setHeader(&head);

    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;
}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE])
{
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    union Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, (char *)"RelName", relNameAttr, EQ);

    if (relcatRecId.block == -1 && relcatRecId.slot == -1) {
        return E_RELNOTEXIST;
    }

    RecBuffer relCatBlock(relcatRecId.block);
    union Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, relcatRecId.slot);

    int firstBlock = relCatRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numAttrs = relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    int currBlockNum = firstBlock;

    while (currBlockNum != -1) {
        RecBuffer currBlock(currBlockNum);
        HeadInfo head;
        currBlock.getHeader(&head);

        int nextBlock = head.rblock;

        currBlock.releaseBlock();
        currBlockNum = nextBlock;
    }

    for (int i = 0; i < numAttrs; i++) {
        RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

        RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)"RelName", relNameAttr, EQ);

        if (attrcatRecId.block != -1 && attrcatRecId.slot != -1) {
            RecBuffer attrCatBlock(attrcatRecId.block);
            HeadInfo head;
            attrCatBlock.getHeader(&head);

            unsigned char slotMap[head.numSlots];
            attrCatBlock.getSlotMap(slotMap);

            slotMap[attrcatRecId.slot] = SLOT_UNOCCUPIED;
            attrCatBlock.setSlotMap(slotMap);

            head.numEntries--;
            attrCatBlock.setHeader(&head);

            if (head.numEntries == 0) {
                int lblock = head.lblock;
                int rblock = head.rblock;

                if (lblock != -1) {
                    RecBuffer prevBlock(lblock);
                    HeadInfo prevHead;
                    prevBlock.getHeader(&prevHead);

                    prevHead.rblock = rblock;
                    prevBlock.setHeader(&prevHead);
                } else {
                    RelCatEntry attrCatRelEntry;
                    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);

                    attrCatRelEntry.firstBlk = rblock;
                    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);
                }

                if (rblock != -1) {
                    RecBuffer nextBlock(rblock);
                    HeadInfo nextHead;
                    nextBlock.getHeader(&nextHead);

                    nextHead.lblock = lblock;
                    nextBlock.setHeader(&nextHead);
                } else {
                    RelCatEntry attrCatRelEntry;
                    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);

                    attrCatRelEntry.lastBlk = lblock;
                    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);
                }

                attrCatBlock.releaseBlock();
            }

            RelCatEntry attrCatRelEntry;
            RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);

            attrCatRelEntry.numRecs--;
            RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatRelEntry);
        }
    }

    RecBuffer relCatBlockForDelete(relcatRecId.block);
    HeadInfo relCatHead;
    relCatBlockForDelete.getHeader(&relCatHead);

    unsigned char relCatSlotMap[relCatHead.numSlots];
    relCatBlockForDelete.getSlotMap(relCatSlotMap);

    relCatSlotMap[relcatRecId.slot] = SLOT_UNOCCUPIED;
    relCatBlockForDelete.setSlotMap(relCatSlotMap);

    relCatHead.numEntries--;
    relCatBlockForDelete.setHeader(&relCatHead);

    RelCatEntry relCatRelEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatRelEntry);

    relCatRelEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatRelEntry);

    return SUCCESS;
}
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
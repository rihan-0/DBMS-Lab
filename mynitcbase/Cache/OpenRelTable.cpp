#include "OpenRelTable.h"
#include <cstdlib>
#include <cstring>


OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable() {
    for (int i = 0; i < MAX_OPEN; ++i) {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
        tableMetaInfo[i].free = true;
    }

    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

    struct RelCacheEntry relCacheEntry;

    RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);

    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;
    relCacheEntry.dirty = false;
    relCacheEntry.searchIndex = {-1, -1};

    RelCacheTable::relCache[RELCAT_RELID] =
        (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));

    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);

    struct RelCacheEntry attrCacheEntry;

    RelCacheTable::recordToRelCatEntry(relCatRecord,&attrCacheEntry.relCatEntry);

    attrCacheEntry.recId.block = RELCAT_BLOCK;
    attrCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
    attrCacheEntry.dirty = false;
    attrCacheEntry.searchIndex = {-1, -1};

    RelCacheTable::relCache[ATTRCAT_RELID] =
        (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));

    *(RelCacheTable::relCache[ATTRCAT_RELID]) = attrCacheEntry;

    RecBuffer attrCatBlock(ATTRCAT_BLOCK);

    AttrCacheEntry *relCatAttrHead = nullptr;
    AttrCacheEntry *relCatAttrTail = nullptr;

    for (int i = 0; i < 6; ++i) {
        Attribute attrRec[ATTRCAT_NO_ATTRS];

        attrCatBlock.getRecord(attrRec, i);

        AttrCacheEntry *newEntry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));

        AttrCacheTable::recordToAttrCatEntry(attrRec,&newEntry->attrCatEntry);

        newEntry->dirty = false;
        newEntry->recId.block = ATTRCAT_BLOCK;
        newEntry->recId.slot = i;
        newEntry->searchIndex = {-1, -1};
        newEntry->next = nullptr;

        if (relCatAttrHead == nullptr) {
            relCatAttrHead = newEntry;
            relCatAttrTail = newEntry;
        } else {
            relCatAttrTail->next = newEntry;
            relCatAttrTail = newEntry;
        }
    }

    AttrCacheTable::attrCache[RELCAT_RELID] = relCatAttrHead;

    AttrCacheEntry *attrCatAttrHead = nullptr;
    AttrCacheEntry *attrCatAttrTail = nullptr;

    for (int i = 6; i < 12; ++i) {
        Attribute attrRec[ATTRCAT_NO_ATTRS];

        attrCatBlock.getRecord(attrRec, i);

        AttrCacheEntry *newEntry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));

        AttrCacheTable::recordToAttrCatEntry(attrRec,&newEntry->attrCatEntry);

        newEntry->dirty = false;
        newEntry->recId.block = ATTRCAT_BLOCK;
        newEntry->recId.slot = i;
        newEntry->searchIndex = {-1, -1};
        newEntry->next = nullptr;

        if (attrCatAttrHead == nullptr) {
            attrCatAttrHead = newEntry;
            attrCatAttrTail = newEntry;
        } else {
            attrCatAttrTail->next = newEntry;
            attrCatAttrTail = newEntry;
        }
    }

    AttrCacheTable::attrCache[ATTRCAT_RELID] = attrCatAttrHead;

    tableMetaInfo[RELCAT_RELID].free = false;
    strcpy((char*)tableMetaInfo[RELCAT_RELID].relName,RELCAT_RELNAME);

    tableMetaInfo[ATTRCAT_RELID].free = false;
    strcpy((char*)tableMetaInfo[ATTRCAT_RELID].relName,ATTRCAT_RELNAME);
}


OpenRelTable::~OpenRelTable()
{
    for (int i = 2; i < MAX_OPEN; ++i) {
        if (!tableMetaInfo[i].free) {
            OpenRelTable::closeRel(i);
        }
    }

    for (int i = 0; i < 2; ++i) {
        if (RelCacheTable::relCache[i] != nullptr) {
            free(RelCacheTable::relCache[i]);
            RelCacheTable::relCache[i] = nullptr;
        }

        AttrCacheEntry *curr = AttrCacheTable::attrCache[i];

        while (curr != nullptr) {
            AttrCacheEntry *next = curr->next;
            free(curr);
            curr = next;
        }

        AttrCacheTable::attrCache[i] = nullptr;
    }
}

int OpenRelTable::closeRel(int relId) {
    if (relId == RELCAT_RELID || relId == ATTRCAT_RELID) {
        return E_NOTPERMITTED;
    }

    if (relId < 0 || relId >= MAX_OPEN) {
        return E_OUTOFBOUND;
    }

    if (tableMetaInfo[relId].free) {
        return E_RELNOTOPEN;
    }

    free(RelCacheTable::relCache[relId]);
    RelCacheTable::relCache[relId] = nullptr;

    AttrCacheEntry *head = AttrCacheTable::attrCache[relId];

    while (head != nullptr) {
        AttrCacheEntry *next = head->next;
        free(head);
        head = next;
    }

    AttrCacheTable::attrCache[relId] = nullptr;

    tableMetaInfo[relId].free = true;

    return SUCCESS;
}

int OpenRelTable::getFreeOpenRelTableEntry() {
    for (int i = 2; i < MAX_OPEN; ++i) {
        if (tableMetaInfo[i].free) {
            return i;
        }
    }
    return E_CACHEFULL;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {
    for (int i = 0; i < MAX_OPEN; ++i) {
        if (!tableMetaInfo[i].free && strcmp((char*)tableMetaInfo[i].relName,(char*)relName) == 0) {
            return i;
        }
    }

    return E_RELNOTOPEN;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]) {
    int relId = OpenRelTable::getRelId(relName);

    if (relId != E_RELNOTOPEN) {
        return relId;
    }

    relId = OpenRelTable::getFreeOpenRelTableEntry();

    if (relId == E_CACHEFULL) {
        return E_CACHEFULL;
    }

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute attrVal;
    strcpy(attrVal.sVal, (char*)relName);

    RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME,attrVal,EQ);

    if (relcatRecId.block == -1 && relcatRecId.slot == -1) {
        return E_RELNOTEXIST;
    }

    RecBuffer relCatBlock(relcatRecId.block);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    relCatBlock.getRecord(relCatRecord, relcatRecId.slot);

    RelCacheEntry *relCacheEntry = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));

    RelCacheTable::recordToRelCatEntry(relCatRecord,&(relCacheEntry->relCatEntry));

    relCacheEntry->recId = relcatRecId;
    relCacheEntry->dirty = false;
    relCacheEntry->searchIndex = {-1, -1};

    RelCacheTable::relCache[relId] = relCacheEntry;

    AttrCacheEntry *listHead = nullptr;
    AttrCacheEntry *last = nullptr;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    while (true) {
        RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID,(char*)ATTRCAT_ATTR_RELNAME,attrVal,EQ);

        if (attrcatRecId.block == -1 && attrcatRecId.slot == -1) {
            break;
        }

        RecBuffer attrCatBlock(attrcatRecId.block);
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

        attrCatBlock.getRecord(attrCatRecord, attrcatRecId.slot);

        AttrCacheEntry *attrCacheEntry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));

        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&(attrCacheEntry->attrCatEntry));
        attrCacheEntry->recId = attrcatRecId;
        attrCacheEntry->dirty = false;
        attrCacheEntry->searchIndex = {-1, -1};
        attrCacheEntry->next = nullptr;

        if (listHead == nullptr) {
            listHead = attrCacheEntry;
            last = attrCacheEntry;
        } else {
            last->next = attrCacheEntry;
            last = attrCacheEntry;
        }
    }

    AttrCacheTable::attrCache[relId] = listHead;

    tableMetaInfo[relId].free = false;

    strcpy((char*)tableMetaInfo[relId].relName,(char*)relName);
    return relId;
}
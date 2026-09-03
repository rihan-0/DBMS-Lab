#include "OpenRelTable.h"
#include <cstdlib>
#include <cstring>

OpenRelTable::OpenRelTable()
{
    for(int i=0;i<MAX_OPEN;i++)
    {
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
    }

    RecBuffer relCatBlock(RELCAT_BLOCK);
    
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord,RELCAT_SLOTNUM_FOR_RELCAT);
    
    struct RelCacheEntry relCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;
    relCacheEntry.dirty = false;
    relCacheEntry.searchIndex = {-1, -1};
    
    RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;
    
    relCatBlock.getRecord(relCatRecord,RELCAT_SLOTNUM_FOR_ATTRCAT);
    RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
    relCacheEntry.dirty = false;
    relCacheEntry.searchIndex = {-1, -1};
    
    RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;
    
    //excercise stage 3 q1
    relCatBlock.getRecord(relCatRecord,2);
    RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = 2;

    RelCacheTable::relCache[2] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[2]) = relCacheEntry;
    //End of Stage q1

    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    struct AttrCacheEntry* head = NULL;
    struct AttrCacheEntry* temp=NULL;
    for(int i=0;i<6;i++)
    {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord,i);
        
        struct AttrCacheEntry attrCacheEntry; 
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry.attrCatEntry);
        attrCacheEntry.recId.block = RELCAT_BLOCK;
        attrCacheEntry.recId.slot = i;
        attrCacheEntry.next = NULL;
        
        struct AttrCacheEntry* temp2 = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        *(temp2) = attrCacheEntry;

        if(!head)
        {
            head = temp2;
            temp = temp2;
        }
        else
        {
            temp->next = temp2;
            temp = temp->next;
        }
    }
    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    head = NULL;
    temp = NULL;
    for(int i=6;i<12;i++)
    {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord,i);
        
        struct AttrCacheEntry attrCacheEntry; 
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry.attrCatEntry);
        attrCacheEntry.recId.block = ATTRCAT_BLOCK;
        attrCacheEntry.recId.slot = i;
        attrCacheEntry.next = NULL;
        
        struct AttrCacheEntry* temp2 = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        *(temp2) = attrCacheEntry;

        if(!head)
        {
            head = temp2;
            temp = temp2;
        }
        else
        {
            temp->next = temp2;
            temp = temp->next;
        }
    }
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

    //Excercise stage 3 q1
    head = NULL;
    temp = NULL;
    for(int i=12;i<16;i++)
    {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord,i);
        
        struct AttrCacheEntry attrCacheEntry; 
        AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry.attrCatEntry);
        attrCacheEntry.recId.block = ATTRCAT_BLOCK;
        attrCacheEntry.recId.slot = i;
        attrCacheEntry.next = NULL;
        
        struct AttrCacheEntry* temp2 = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        *(temp2) = attrCacheEntry;

        if(!head)
        {
            head = temp2;
            temp = temp2;
        }
        else
        {
            temp->next = temp2;
            temp = temp->next;
        }
    }
    AttrCacheTable::attrCache[2] = head;

    //End of stage 3 q1
}

int OpenRelTable::getRelId(char relname[ATTR_SIZE])
{
    if(strcmp(relname,RELCAT_RELNAME)==0)
        return RELCAT_RELID;
    if(strcmp(relname,ATTRCAT_RELNAME)==0)
        return ATTRCAT_RELID;
    //stage 4 q1
    if(strcmp(relname,"Students")==0)
        return 2;
    return E_RELNOTOPEN;
}

OpenRelTable::~OpenRelTable()
{
    for(int i=0;i<MAX_OPEN;i++)
    {
        if(RelCacheTable::relCache[i] != nullptr)
        {
            free(RelCacheTable::relCache[i]);
            RelCacheTable::relCache[i] = nullptr;
        }
        struct AttrCacheEntry* curr = AttrCacheTable::attrCache[i];
        while(curr!=nullptr)
        {
            struct AttrCacheEntry* next = curr->next;
            free(curr);
            curr = next;
        }
        AttrCacheTable::attrCache[i] = nullptr;
    }
}
#include "Algebra.h"
#include <iostream>
#include <cstring>

bool isNumber(char *str) {
  int len;
  float ignore;
  int ret = sscanf(str,"%f %n", &ignore, &len);
  return ret == 1 && len == strlen(str);
}

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE])
{
    int srcRelId = OpenRelTable::getRelId(srcRel);
    if (srcRelId == E_RELNOTOPEN){
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry;
    int ret = AttrCacheTable::getAttrCatEntry(srcRelId,attr,&attrCatEntry);
    if(ret != SUCCESS)
        return ret;
    
    int type = attrCatEntry.attrType;
    Attribute attrVal;
    if(type == NUMBER)
    {
        if(isNumber(strVal))
        {
            attrVal.nVal = atof(strVal);
        }
        else
        {
            return E_ATTRTYPEMISMATCH;
        }
    }
    else if(type==STRING)
    {
        strcpy(attrVal.sVal,strVal);
    }

    RelCacheTable::resetSearchIndex(srcRelId);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    //Stage 4 way of printing contents of relation
    printf("|");
    for(int i=0;i<relCatEntry.numAttrs;i++)
    {
        AttrCatEntry battrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId,i,&battrCatEntry);
        printf(" %s |",battrCatEntry.attrName);
    }
    printf("\n");
    while(true)
    {
        RecId searchRes = BlockAccess::linearSearch(srcRelId,attr,attrVal,op);
        if(searchRes.block != -1 && searchRes.slot != -1)
        {
            RecBuffer recBuffer(searchRes.block);
            union Attribute record[relCatEntry.numAttrs];
            recBuffer.getRecord(record,searchRes.slot);

            printf("|");
            for(int i=0;i<relCatEntry.numAttrs;i++)
            {
                AttrCatEntry tattrCatEntry;
                AttrCacheTable::getAttrCatEntry(srcRelId, i , &tattrCatEntry);
                if (tattrCatEntry.attrType == NUMBER) {
                    printf(" %d |", (int)record[i].nVal);
                } else {
                    printf(" %s |", record[i].sVal);
                }
            }
            printf("\n");
        }
        else
        {
            break;
        }
    }
    return SUCCESS;
}
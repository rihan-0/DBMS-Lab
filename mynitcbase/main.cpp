#include "Buffer/StaticBuffer.h"
#include "Buffer/BlockBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include<iostream>
#include<cstring>
using namespace std;

void changeAttrName(char* changeFrmRel,char* changeFrm,char* changeTo)
{
    bool relFound = false;
    bool attrFound = false;

    int curBlock = ATTRCAT_BLOCK;

    while(curBlock!=-1)
    {
        RecBuffer attrCatBuffer(curBlock);
        HeadInfo header;
        attrCatBuffer.getHeader(&header);
        int m = header.numEntries;
        for(int j=0;j<m;j++)
        {
            Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
            attrCatBuffer.getRecord(attrCatRecord,j);
            if(strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,changeFrmRel)==0)
            {
                relFound = true;
                if(strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,changeFrm)==0)
                {
                    attrFound = true;
                    strncpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,changeTo,15);
                    attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal[15] = '\0';
                    unsigned char buffer[BLOCK_SIZE];
                    Disk::readBlock(buffer,curBlock);
                    unsigned char *slotPointer = buffer+(HEADER_SIZE + 20 + (96*j));
                    memcpy(slotPointer,attrCatRecord,96);
                    Disk::writeBlock(buffer,curBlock);
                    break;
                }
            }
        }
        if(relFound && attrFound)
            break;
        curBlock = header.rblock;
    }

    if(!relFound)
    {
        printf("No such Relation Found!\n");
    }
    else if(!attrFound)
    {
        printf("No such Attribute Found in the relation!\n");
    }
    else
    {
        printf("Attribute Name Changed!\n");
    }
}

void printSchema()
{
    RecBuffer relCatBuffer(RELCAT_BLOCK);
    HeadInfo relCatHeader;
    relCatBuffer.getHeader(&relCatHeader);
    int n = relCatHeader.numEntries;

    for(int i=0;i<n;i++)
    {
        Attribute relCatRecord[RELCAT_NO_ATTRS];
        relCatBuffer.getRecord(relCatRecord,i);
        printf("Relation: %s\n",relCatRecord[RELCAT_REL_NAME_INDEX].sVal);
        int curBlock = ATTRCAT_BLOCK;
        while(curBlock!=-1)
        {
            RecBuffer attrCatBuffer(curBlock);
            HeadInfo header;
            attrCatBuffer.getHeader(&header);
            int m = header.numEntries;
            for(int j=0;j<m;j++)
            {
                Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
                attrCatBuffer.getRecord(attrCatRecord,j);
                if(strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relCatRecord[RELCAT_REL_NAME_INDEX].sVal)==0)
                {
                    const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
                    printf("  %s: %s\n",attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,attrType);
                }
            }
            curBlock = header.rblock;
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    /* Initialize the Run Copy of Disk */
    Disk disk_run;
    StaticBuffer buffer;
    OpenRelTable cache;

    /*char Student[16] = "Students";
    char Class[16] = "Class";
    char Batch[16] = "Batch";

    changeAttrName(Student,Class,Batch);
    
    printSchema();*/

    for(int i=0;i<3;i++)
    {
        RelCatEntry relCatBuffer;
        RelCacheTable::getRelCatEntry(i,&relCatBuffer);
        printf("Relation: %s\n",relCatBuffer.relName);

        for(int j=0;j<relCatBuffer.numAttrs;j++)
        {
            AttrCatEntry attrCatBuffer;
            AttrCacheTable::getAttrCatEntry(i,j,&attrCatBuffer);
            const char *attrType = attrCatBuffer.attrType == NUMBER ? "NUM" : "STR";
            printf("    %s: %s\n",attrCatBuffer.attrName,attrType);
        }
    }

    return 0;
    //return FrontendInterface::handleFrontend(argc, argv);
}
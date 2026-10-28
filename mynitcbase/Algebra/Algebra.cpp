#include "Algebra.h"
#include <iostream>
#include <cstring>

bool isNumber(char *str) {
  int len;
  float ignore;
  int ret = sscanf(str,"%f %n", &ignore, &len);
  return ret == 1 && len == strlen(str);
}

int Algebra::insert(char relName[ATTR_SIZE], int nAttrs, char record[][ATTR_SIZE]) {
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    int relId = OpenRelTable::getRelId(relName);

    if (relId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    if (relCatEntry.numAttrs != nAttrs) {
        return E_NATTRMISMATCH;
    }

    union Attribute recordValues[nAttrs];

    for (int i = 0; i < nAttrs; i++) {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, i, &attrCatEntry);

        int type = attrCatEntry.attrType;

        if (type == NUMBER) {
            if (isNumber(record[i])) {
                recordValues[i].nVal = atof(record[i]);
            } else {
                return E_ATTRTYPEMISMATCH;
            }
        } else if (type == STRING) {
            strcpy(recordValues[i].sVal, record[i]);
        }
    }

    int retVal = BlockAccess::insert(relId, recordValues);
    return retVal;
}

int Algebra::select(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], char attr[ATTR_SIZE], int op, char strVal[ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);

    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    AttrCatEntry attrCatEntry;
    int retVal = AttrCacheTable::getAttrCatEntry(srcRelId, attr, &attrCatEntry);

    if (retVal != SUCCESS) {
        return E_ATTRNOTEXIST;
    }

    Attribute attrVal;
    int type = attrCatEntry.attrType;

    if (type == NUMBER) {
        if (isNumber(strVal)) {
            attrVal.nVal = atof(strVal);
        } else {
            return E_ATTRTYPEMISMATCH;
        }
    } else if (type == STRING) {
        strcpy(attrVal.sVal, strVal);
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    int src_nAttrs = relCatEntry.numAttrs;
    char attr_names[src_nAttrs][ATTR_SIZE];
    int attr_types[src_nAttrs];

    for (int i = 0; i < src_nAttrs; i++) {
        AttrCatEntry srcAttrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &srcAttrCatEntry);

        strcpy(attr_names[i], srcAttrCatEntry.attrName);
        attr_types[i] = srcAttrCatEntry.attrType;
    }

    retVal = Schema::createRel(targetRel, src_nAttrs, attr_names, attr_types);

    if (retVal != SUCCESS) {
        return retVal;
    }

    int targetRelId = OpenRelTable::openRel(targetRel);

    if (targetRelId < 0) {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    Attribute record[src_nAttrs];

    RelCacheTable::resetSearchIndex(srcRelId);

    while (BlockAccess::search(srcRelId, record, attr, attrVal, op) == SUCCESS) {
        int ret = BlockAccess::insert(targetRelId, record);

        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    Schema::closeRel(targetRel);
    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);

    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    int numAttrs = relCatEntry.numAttrs;

    char attrNames[numAttrs][ATTR_SIZE];
    int attrTypes[numAttrs];

    for (int i = 0; i < numAttrs; i++) {
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(srcRelId, i, &attrCatEntry);

        strcpy(attrNames[i], attrCatEntry.attrName);
        attrTypes[i] = attrCatEntry.attrType;
    }

    int retVal = Schema::createRel(targetRel, numAttrs, attrNames, attrTypes);

    if (retVal != SUCCESS) {
        return retVal;
    }

    int targetRelId = OpenRelTable::openRel(targetRel);

    if (targetRelId < 0) {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[numAttrs];

    while (BlockAccess::project(srcRelId, record) == SUCCESS) {
        int ret = BlockAccess::insert(targetRelId, record);

        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    Schema::closeRel(targetRel);
    return SUCCESS;
}

int Algebra::project(char srcRel[ATTR_SIZE], char targetRel[ATTR_SIZE], int tar_nAttrs, char tar_Attrs[][ATTR_SIZE]) {
    int srcRelId = OpenRelTable::getRelId(srcRel);

    if (srcRelId == E_RELNOTOPEN) {
        return E_RELNOTOPEN;
    }

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(srcRelId, &relCatEntry);

    int src_nAttrs = relCatEntry.numAttrs;

    int attr_offset[tar_nAttrs];
    int attr_types[tar_nAttrs];

    for (int i = 0; i < tar_nAttrs; i++) {
        AttrCatEntry attrCatEntry;
        int retVal = AttrCacheTable::getAttrCatEntry(srcRelId, tar_Attrs[i], &attrCatEntry);

        if (retVal != SUCCESS) {
            return E_ATTRNOTEXIST;
        }

        attr_offset[i] = attrCatEntry.offset;
        attr_types[i] = attrCatEntry.attrType;
    }

    int retVal = Schema::createRel(targetRel, tar_nAttrs, tar_Attrs, attr_types);

    if (retVal != SUCCESS) {
        return retVal;
    }

    int targetRelId = OpenRelTable::openRel(targetRel);

    if (targetRelId < 0) {
        Schema::deleteRel(targetRel);
        return targetRelId;
    }

    RelCacheTable::resetSearchIndex(srcRelId);

    Attribute record[src_nAttrs];

    while (BlockAccess::project(srcRelId, record) == SUCCESS) {
        Attribute proj_record[tar_nAttrs];

        for (int i = 0; i < tar_nAttrs; i++) {
            proj_record[i] = record[attr_offset[i]];
        }

        int ret = BlockAccess::insert(targetRelId, proj_record);

        if (ret != SUCCESS) {
            Schema::closeRel(targetRel);
            Schema::deleteRel(targetRel);
            return ret;
        }
    }

    Schema::closeRel(targetRel);
    return SUCCESS;
}
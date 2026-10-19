#include "AttrCacheTable.h"
#include<iostream>
#include <cstring>

AttrCacheEntry* AttrCacheTable::attrCache[MAX_OPEN];


/*
Returns the attrOffset-th attribute for the relation corresponding to relId.

NOTE: this function expects the caller to allocate memory for *attrCatBuf
*/
int AttrCacheTable::getAttrCatEntry(
    int relId,
    int attrOffset,
    AttrCatEntry* attrCatBuf) {

    // check if relId is valid
    if (relId < 0 || relId >= MAX_OPEN) {
        return E_OUTOFBOUND;
    }

    // check if relation is open
    if (attrCache[relId] == nullptr) {
       
        return E_RELNOTOPEN;
    }

    // traverse the linked list of attribute cache entries
    for (AttrCacheEntry* entry = attrCache[relId];
         entry != nullptr;
         entry = entry->next) {

        if (entry->attrCatEntry.offset == attrOffset) {

            // copy attribute catalog entry
            *attrCatBuf = entry->attrCatEntry;

            return SUCCESS;
        }
    }

    // there is no attribute at this offset
    
    return E_ATTRNOTEXIST;
}


/*
Converts an attribute catalog record to AttrCatEntry struct.

We get the record as Attribute[] from the BlockBuffer.getRecord()
function. This function converts that to an AttrCatEntry type.
*/
void AttrCacheTable::recordToAttrCatEntry(
    union Attribute record[ATTRCAT_NO_ATTRS],
    AttrCatEntry* attrCatEntry) {

    strcpy(
        attrCatEntry->relName,
        record[ATTRCAT_REL_NAME_INDEX].sVal
    );

    strcpy(
        attrCatEntry->attrName,
        record[ATTRCAT_ATTR_NAME_INDEX].sVal
    );

    attrCatEntry->attrType =
        (int)record[ATTRCAT_ATTR_TYPE_INDEX].nVal;

    attrCatEntry->offset =
        (int)record[ATTRCAT_OFFSET_INDEX].nVal;

    
}

/* returns the attribute with name `attrName` for the relation corresponding to relId
NOTE: this function expects the caller to allocate memory for `*attrCatBuf`
*/
int AttrCacheTable::getAttrCatEntry(int relId, char attrName[ATTR_SIZE], AttrCatEntry* attrCatBuf) {

  // check that relId is valid and corresponds to an open relation

  // iterate over the entries in the attribute cache and set attrCatBuf to the entry that
  //    matches attrName

  // no attribute with name attrName for the relation
    if(relId<0||relId>=MAX_OPEN)
    return E_OUTOFBOUND;

    if(attrCache[relId]==nullptr)
    {
        
    return E_RELNOTOPEN;
    }
    for(AttrCacheEntry* entry=attrCache[relId];entry!=nullptr;entry=entry->next)
    {
        if(strcmp(entry->attrCatEntry.attrName,attrName)==0)
        {
            *attrCatBuf=entry->attrCatEntry;
            return SUCCESS;
        }
    }

  return E_ATTRNOTEXIST;
}


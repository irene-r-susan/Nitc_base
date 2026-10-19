#include "BlockAccess.h"

#include <cstring>


RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op) {
    // 1. Fetch the attribute catalog entry ONCE before entering the search loop
    AttrCatEntry attrCatEntry;
    int attrStatus = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
    if (attrStatus != SUCCESS) {
        return RecId{-1, -1}; // Attribute does not exist in this relation
    }

    // 2. Get the previous search index of relId from relation cache
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block, slot;
    if (prevRecId.block == -1 && prevRecId.slot == -1) {
        // First search call: Start from the first block, slot 0
        RelCatEntry relCatEntry;
        RelCacheTable::getRelCatEntry(relId, &relCatEntry);
        block = relCatEntry.firstBlk;
        slot = 0;
    } else {
        // Subsequent call: Start from the slot immediately after the last hit
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    // 3. Iterate over blocks and slots to find matching record
    while (block != -1) {
        RecBuffer recBuffer(block);
        
        HeadInfo head;
        recBuffer.getHeader(&head);

        unsigned char slotMap[head.numSlots];
        recBuffer.getSlotMap(slotMap);

        // Move to right block if slot exceeds block capacity
        if (slot >= head.numSlots) {
            block = head.rblock;
            slot = 0;
            continue;
        }

        // Skip unoccupied slots
        if (slotMap[slot] == SLOT_UNOCCUPIED) {
            slot++;
            continue;
        }

        // Read record and compare attribute
        union Attribute record[head.numAttrs];
        recBuffer.getRecord(record, slot);

        union Attribute currentAttrVal = record[attrCatEntry.offset];
        int cmpVal = compareAttrs(currentAttrVal, attrVal, attrCatEntry.attrType);

        // Check if condition is satisfied
        if (
            (op == NE && cmpVal != 0) ||
            (op == LT && cmpVal < 0) ||
            (op == LE && cmpVal <= 0) ||
            (op == EQ && cmpVal == 0) ||
            (op == GT && cmpVal > 0) ||
            (op == GE && cmpVal >= 0)
        ) {
            RecId searchIndex = {block, slot};
            RelCacheTable::setSearchIndex(relId, &searchIndex);
            return searchIndex;
        }

        slot++;
    }

    // Reset search index when search reaches the end without matching
    RecId resetIndex = {-1, -1};
    RelCacheTable::setSearchIndex(relId, &resetIndex);

    return RecId{-1, -1};
}


int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {
    // 1. Reset the search index for Relation Catalog
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;
    strcpy(newRelationName.sVal, newName);

    // 2. Search Relation Catalog to check if a relation with newName already exists
    // Search condition: "RelName" == newName
    RecId targetRecId = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, newRelationName, EQ);

    // If relation with newName already exists
    if (targetRecId.block != -1 && targetRecId.slot != -1) {
        return E_RELEXIST;
    }

    // 3. Reset the search index for Relation Catalog again
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;
    strcpy(oldRelationName.sVal, oldName);

    // 4. Search Relation Catalog to locate the relation entry for oldName
    targetRecId = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, oldRelationName, EQ);

    // If relation with oldName does not exist
    if (targetRecId.block == -1 && targetRecId.slot == -1) {
        return E_RELNOTEXIST;
    }

    // 5. Fetch the relation catalog record and update its "RelName" attribute
    RecBuffer relCatBlock(targetRecId.block);
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBlock.getRecord(relCatRecord, targetRecId.slot);

    // Update RelName attribute (RELCAT_REL_NAME_INDEX) with newName
    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, newName);
    relCatBlock.setRecord(relCatRecord, targetRecId.slot);

    // Get the total number of attributes for this relation from the record
    int numAttrs = relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    // 6. Update all corresponding attribute catalog entries to have the new relation name
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    for (int i = 0; i < numAttrs; i++) {
        // Find the next attribute catalog entry matching relName = oldName
        RecId attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, oldRelationName, EQ);

        if (attrCatRecId.block != -1 && attrCatRecId.slot != -1) {
            RecBuffer attrCatBlock(attrCatRecId.block);
            Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
            attrCatBlock.getRecord(attrCatRecord, attrCatRecId.slot);

            // Update relName field (ATTRCAT_REL_NAME_INDEX) to newName
            strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, newName);
            attrCatBlock.setRecord(attrCatRecord, attrCatRecId.slot);
        }
    }

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE]) {
    // 1. Reset the search index of the relation catalog
    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    // 2. Search for the relation with name relName in relation catalog
    RecId relCatRecId = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, EQ);

    // If relation with name relName does not exist
    if (relCatRecId.block == -1 && relCatRecId.slot == -1) {
        return E_RELNOTEXIST;
    }

    // 3. Reset the search index of the attribute catalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    // 4. Iterate over all Attribute Catalog entries corresponding to relName
    while (true) {
        // Search attribute catalog for entries matching relName
        RecId attrRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

        // If no more attributes found
        if (attrRecId.block == -1 && attrRecId.slot == -1) {
            break;
        }

        // Get the record from attribute catalog
        RecBuffer attrCatBlock(attrRecId.block);
        attrCatBlock.getRecord(attrCatEntryRecord, attrRecId.slot);

        // Check if this attribute matches oldName
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0) {
            attrToRenameRecId = attrRecId;
        }

        // Check if an attribute with newName already exists in this relation
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0) {
            return E_ATTREXIST;
        }
    }

    // 5. If attribute to rename was not found
    if (attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1) {
        return E_ATTRNOTEXIST;
    }

    // 6. Update the AttrName field in Attribute Catalog
    RecBuffer attrCatBlock(attrToRenameRecId.block);
    attrCatBlock.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    // Update AttrName (ATTRCAT_ATTR_NAME_INDEX) with newName
    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    attrCatBlock.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    return SUCCESS;
}
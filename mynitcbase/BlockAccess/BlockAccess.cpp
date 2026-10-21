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


int BlockAccess::insert(int relId, Attribute *record) {
    // get the relation catalog entry from relation cache
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)
	RelCatEntry relCatEntry;
	RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int blockNum = relCatEntry.firstBlk;

    // rec_id will be used to store where the new record will be inserted
    RecId rec_id = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;

	// block number of the last element in the linked list = -1 
    int prevBlockNum = -1;

	// Traversing the linked list of existing record blocks of the relation
	// until a free slot is found OR until the end of the list is reached

    while (blockNum != -1) {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
		RecBuffer blockBuffer (blockNum);

        // get header of block(blockNum) using RecBuffer::getHeader() function
		HeadInfo blockHeader;
		blockBuffer.getHeader(&blockHeader);

        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function
		int numSlots = blockHeader.numSlots;
		unsigned char slotMap [numSlots];
		blockBuffer.getSlotMap(slotMap);

        // search for free slot in the block 'blockNum' and store it's rec-id in rec_id
        // (Free slot can be found by iterating over the slot map of the block)
		int slotIndex = 0;
		for (; slotIndex < numSlots; slotIndex++) {
        	// if a free slot is found, set rec_id and discontinue the traversal
           	// of the linked list of record blocks (break from the loop) 
			//* slot map stores SLOT_UNOCCUPIED if slot is free and SLOT_OCCUPIED if slot is occupied
			if (slotMap[slotIndex] == SLOT_UNOCCUPIED) {
				rec_id = RecId{blockNum, slotIndex};
				break;
			}
		}

		if (rec_id.block != -1&&rec_id.slot!= -1) break;

        /* otherwise, continue to check the next block by updating the
           block numbers as follows:
              update prevBlockNum = blockNum
              update blockNum = header.rblock (next element in the linked list of record blocks)
        */
	   prevBlockNum = blockNum;
	   blockNum = blockHeader.rblock;
    }

    //  if no free slot is found in existing record blocks (rec_id = {-1, -1})
	if (rec_id.block == -1 && rec_id.slot==-1)
    {
        // if relation is RELCAT, do not allocate any more blocks
        //     return E_MAXRELATIONS;
		if (relId == RELCAT_RELID) return E_MAXRELATIONS;

        // Otherwise,
        // get a new record block (using the appropriate RecBuffer constructor!)
		RecBuffer blockBuffer;

        // get the block number of the newly allocated block
        // (use BlockBuffer::getBlockNum() function)
        blockNum = blockBuffer.getBlockNum();
		
		// let ret be the return value of getBlockNum() function call
        if (blockNum == E_DISKFULL) return E_DISKFULL;

        // Assign rec_id.block = new block number(i.e. ret) and rec_id.slot = 0
		rec_id = RecId {blockNum, 0};

		// TODO: set the header of the new record block such that it links with
		// TODO: existing record blocks of the relation
		// TODO: set the block's header as follows:
		// blockType: REC, pblock: -1
		// lblock = -1 (if linked list of existing record blocks was empty
		// 				i.e this is the first insertion into the relation)
		// 		= prevBlockNum (otherwise),
		// rblock: -1, numEntries: 0,
		// numSlots: numOfSlots, numAttrs: numOfAttributes
		// (use BlockBuffer::setHeader() function)
        
		HeadInfo blockHeader;
		blockHeader.blockType = REC;
		blockHeader.lblock = prevBlockNum, blockHeader.rblock = blockHeader.pblock = -1;
		blockHeader.numAttrs = numOfAttributes, blockHeader.numSlots = numOfSlots, blockHeader.numEntries = 0;

		blockBuffer.setHeader(&blockHeader);
        /*
            set block's slot map with all slots marked as free
            (i.e. store SLOT_UNOCCUPIED for all the entries)
            (use RecBuffer::setSlotMap() function)
        */
	   	unsigned char slotMap [numOfSlots];
		for (int slotIndex = 0; slotIndex < numOfSlots; slotIndex++)
			slotMap[slotIndex] = SLOT_UNOCCUPIED;

		blockBuffer.setSlotMap(slotMap);

        // if prevBlockNum != -1
		if (prevBlockNum != -1)
        {
            // TODO: create a RecBuffer object for prevBlockNum
			RecBuffer prevBlockBuffer (prevBlockNum);

            // TODO: get the header of the block prevBlockNum and
			HeadInfo prevBlockHeader;
			prevBlockBuffer.getHeader(&prevBlockHeader);

            // TODO: update the rblock field of the header to the new block
			prevBlockHeader.rblock = blockNum;
            // number i.e. rec_id.block
            // (use BlockBuffer::setHeader() function)
			prevBlockBuffer.setHeader(&prevBlockHeader);
        }
        else
        {
            // update first block field in the relation catalog entry to the
            // new block (using RelCacheTable::setRelCatEntry() function)
			relCatEntry.firstBlk = blockNum;
			RelCacheTable::setRelCatEntry(relId, &relCatEntry);
        }

        // update last block field in the relation catalog entry to the
        // new block (using RelCacheTable::setRelCatEntry() function)
		relCatEntry.lastBlk = blockNum;
		RelCacheTable::setRelCatEntry(relId, &relCatEntry);
    }

    // create a RecBuffer object for rec_id.block
    RecBuffer blockBuffer (rec_id.block);

	// insert the record into rec_id'th slot using RecBuffer.setRecord())
	blockBuffer.setRecord(record, rec_id.slot);

    /* update the slot map of the block by marking entry of the slot to
       which record was inserted as occupied) */
    // (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
    // (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)
	unsigned char slotmap [numOfSlots];
	blockBuffer.getSlotMap(slotmap);

	slotmap[rec_id.slot] = SLOT_OCCUPIED;
	blockBuffer.setSlotMap(slotmap);

    // increment the numEntries field in the header of the block to
    // which record was inserted
    // (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
	HeadInfo blockHeader;
	blockBuffer.getHeader(&blockHeader);

	blockHeader.numEntries++;
	blockBuffer.setHeader(&blockHeader);

    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
	relCatEntry.numRecs++;
	RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;
}
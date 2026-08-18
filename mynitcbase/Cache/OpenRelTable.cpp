#include "OpenRelTable.h"
#include<bits/stdc++.h>
#include <cstring>
OpenRelTable::OpenRelTable() {

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
  }

  /************ Setting up Relation Cache entries ************/
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK);
  Attribute relCatRecord[RELCAT_NO_ATTRS];

  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);
  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;
  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry)); // allocate this on the heap because we want it to persist outside this function
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
  relCatBlock.getRecord(relCatRecord,RELCAT_SLOTNUM_FOR_ATTRCAT);
  RelCacheTable:: recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
  RelCacheTable :: relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;


  // setup STUDENT catalog relation relation Cache Table
  relCatBlock.getRecord(relCatRecord,2);
  RelCacheTable::recordToRelCatEntry(relCatRecord,&relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = 2;
  RelCacheTable::relCache[2] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable:: relCache[2]) = relCacheEntry;


  /************ Setting up Attribute cache entries ************/
  // (we need to populate attribute cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Attribute Cache Table ****/
  RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

  // iterate through all the attributes of the relation catalog and create a linked
  // list of AttrCacheEntry (slots 0 to 5)
  // for each of the entries, set
  //    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
  //    attrCacheEntry.recId.slot = i   (0 to 5)
  //    and attrCacheEntry.next appropriately
  // NOTE: allocate each entry dynamically using malloc
  // set the next field in the last entry to nullptr

  struct AttrCacheEntry *head,*last;
  for(int i=0;i<6;i++){
    attrCatBlock.getRecord(attrCatRecord,i);
    struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId.block = ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot = i;
    if(i==0){
        head = attrCacheEntry;
        last = attrCacheEntry;
    }
    else{
        last->next = attrCacheEntry;
        last = attrCacheEntry;
    }
  }
  last->next = nullptr;
  AttrCacheTable::attrCache[RELCAT_RELID] = head;



  /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/
  // set up the attributes of the attribute cache similarly.
  // read slots 6-11 from attrCatBlock and initialise recId appropriately
  // set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]

  for(int i=6;i<12;i++){ 
    attrCatBlock.getRecord(attrCatRecord,i);
    struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord,&attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId.block = ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot = i;
    if(i==6){
        head = attrCacheEntry;
        last = attrCacheEntry;
    }
    else{
        last->next = attrCacheEntry;
        last = attrCacheEntry;
    }
  }
  last->next = nullptr;
  AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

  // FOR STUDENT;
  for(int i=12;i<18;i++){
    attrCatBlock.getRecord(attrCatRecord,i);
    struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable:: recordToAttrCatEntry(attrCatRecord,&attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId.block = ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot = i;
    if(i==12){
        head = attrCacheEntry;
        last = attrCacheEntry;
    }
    else{
        last->next = attrCacheEntry;
        last = attrCacheEntry;
    }
  }
  
  last->next = nullptr;
  AttrCacheTable::attrCache[2] = head;

}

OpenRelTable::~OpenRelTable() {
  // free all the memory that you allocated in the constructor
  
}

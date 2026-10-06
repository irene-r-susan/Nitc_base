#include "OpenRelTable.h"
#include <bits/stdc++.h>
#include <cstring>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];


OpenRelTable::OpenRelTable() {

  // Initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i) {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
    tableMetaInfo[i].free=true;
  }

  /************ Setting up Relation Cache entries ************/

  RecBuffer relCatBlock(RELCAT_BLOCK);
  Attribute relCatRecord[RELCAT_NO_ATTRS];

  /**** Relation Catalog in Relation Cache (Slot 0) ****/
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);
  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  relCacheEntry.searchIndex = {-1, -1};

  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;

  /**** Attribute Catalog in Relation Cache (Slot 1) ****/
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;
  relCacheEntry.searchIndex = {-1, -1};



  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry*)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry;

   


  /************ Setting up Attribute Cache entries ************/

  RecBuffer attrCatBlock(ATTRCAT_BLOCK);
  Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
  struct AttrCacheEntry *head = nullptr, *last = nullptr;

  /**** 1. Relation Catalog Attributes (Slots 0 to 5) ****/
  for (int i = 0; i < 6; i++) {
    attrCatBlock.getRecord(attrCatRecord, i);
    struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId.block = ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot = i;
    attrCacheEntry->next = nullptr;
    if (i == 0) {
      head = attrCacheEntry;
      last = attrCacheEntry;
    } else {
      last->next = attrCacheEntry;
      last = attrCacheEntry;
    }
  }
  
  AttrCacheTable::attrCache[RELCAT_RELID] = head;


  /**** 2. Attribute Catalog Attributes (Slots 6 to 11) - WAS MISSING ****/
  head = nullptr;
  last = nullptr;
  for (int i = 6; i < 12; i++) {
    attrCatBlock.getRecord(attrCatRecord, i);
    struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);
    attrCacheEntry->recId.block = ATTRCAT_BLOCK;
    attrCacheEntry->recId.slot = i;
    attrCacheEntry->next = nullptr;
    if (i == 6) {
      head = attrCacheEntry;
      last = attrCacheEntry;
    } else {
      last->next = attrCacheEntry;
      last = attrCacheEntry;
    }
  }  
  AttrCacheTable::attrCache[ATTRCAT_RELID] = head;


 tableMetaInfo[RELCAT_RELID].free=false;
 tableMetaInfo[ATTRCAT_RELID].free=false;
 strcpy(tableMetaInfo[RELCAT_RELID].relName,RELCAT_RELNAME);
 strcpy(tableMetaInfo[ATTRCAT_RELID].relName,ATTRCAT_RELNAME);


}

int OpenRelTable::openRel(char relName[ATTR_SIZE])
{
  int relId=getRelId(relName);
  //check if rel is already open
  if(relId!=E_RELNOTOPEN)
  {
    return relId;
  }

  int freeSlot=getFreeOpenRelTableEntry();
  if(freeSlot==E_CACHEFULL)
  {
    return E_CACHEFULL;
  }

  Attribute attrVal;
  strcpy(attrVal.sVal,relName);
  RelCacheTable::resetSearchIndex(RELCAT_RELID);

  RecId relcatRecId=BlockAccess::linearSearch(RELCAT_RELID,(char*)RELCAT_ATTR_RELNAME,attrVal,EQ);
  if(relcatRecId.block==-1&&relcatRecId.slot==-1)
  {
    return E_RELNOTEXIST;
  }

  RecBuffer relCatBlock(relcatRecId.block);
  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, relcatRecId.slot);

  struct RelCacheEntry* relCacheEntryPtr=(struct RelCacheEntry*)malloc(sizeof(struct RelCacheEntry));
  RelCacheTable::recordToRelCatEntry(relCatRecord,&(relCacheEntryPtr->relCatEntry));
  relCacheEntryPtr->recId=relcatRecId;
  relCacheEntryPtr->searchIndex = {-1, -1};
  RelCacheTable::relCache[freeSlot]=relCacheEntryPtr;

  struct AttrCacheEntry *head = nullptr, *last = nullptr;
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  while (true) {
    RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char*)ATTRCAT_ATTR_RELNAME, attrVal, EQ);
    if (attrcatRecId.block == -1 && attrcatRecId.slot == -1) {
      break;
    }

    RecBuffer attrCatBlock(attrcatRecId.block);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    attrCatBlock.getRecord(attrCatRecord, attrcatRecId.slot);

    struct AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));
    attrCacheEntry->recId = attrcatRecId;
    attrCacheEntry->next = nullptr;

    if (head == nullptr) {
      head = attrCacheEntry;
      last = attrCacheEntry;
    } else {
      last->next = attrCacheEntry;
      last = attrCacheEntry;
    }
  }

  AttrCacheTable::attrCache[freeSlot] = head;

  tableMetaInfo[freeSlot].free = false;
  strcpy(tableMetaInfo[freeSlot].relName, relName);

  return freeSlot;
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

  for(int i=0;i<MAX_OPEN;i++)
  {
    if(!tableMetaInfo[i].free && !strcmp(relName,tableMetaInfo[i].relName))
    return i;
  }
  return E_RELNOTOPEN;
  
}



int OpenRelTable::getFreeOpenRelTableEntry()
{
  for(int i=0;i<MAX_OPEN;i++)
  if(tableMetaInfo[i].free==true)
  return i;
  return E_CACHEFULL;
}


int OpenRelTable::closeRel(int relId)
{
  if(relId==RELCAT_RELID||relId==ATTRCAT_RELID)
  return E_NOTPERMITTED;
  if(relId<0||relId>=MAX_OPEN)
  return E_OUTOFBOUND;
  if(tableMetaInfo[relId].free)
  return E_RELNOTOPEN;

  free(RelCacheTable::relCache[relId]);
  RelCacheTable::relCache[relId] = nullptr;

AttrCacheEntry* curr = AttrCacheTable::attrCache[relId];
  while (curr != nullptr) {
    AttrCacheEntry* next = curr->next;
    free(curr);
    curr = next;
  }
  AttrCacheTable::attrCache[relId] = nullptr;

  tableMetaInfo[relId].free = true;
  return SUCCESS;
}






OpenRelTable::~OpenRelTable() {

for(int i=2;i<MAX_OPEN;i++)
{
  if(!tableMetaInfo[i].free)
  {
    OpenRelTable::closeRel(i);
  }
}


for(int i=0;i<2;i++)
{
  if(RelCacheTable::relCache[i]!=nullptr)
  {
    free(RelCacheTable::relCache[i]);
    RelCacheTable::relCache[i]=nullptr;
  }
   if (AttrCacheTable::attrCache[i] != nullptr) {
      AttrCacheEntry *curr = AttrCacheTable::attrCache[i];
      while (curr != nullptr) {
        AttrCacheEntry *next = curr->next;
        free(curr);
        curr = next;
      }
      AttrCacheTable::attrCache[i] = nullptr;
    }
  tableMetaInfo[i].free=true;
}

  
}
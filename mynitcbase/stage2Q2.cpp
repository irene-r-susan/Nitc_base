#include <iostream>
#include <cstdio>
#include <cstring>
#include "FrontendInterface/FrontendInterface.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "Buffer/StaticBuffer.h"

// Helper function to print relation attributes


void printRelations() {
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  HeadInfo relCatHeader;
  relCatBuffer.getHeader(&relCatHeader);

  for (int i = 0; i < relCatHeader.numEntries; i++) {
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    int attrBlockNum = ATTRCAT_BLOCK;
    while (attrBlockNum != -1) {
      RecBuffer attrCatBuffer(attrBlockNum);
      HeadInfo attrCatHeader;
      attrCatBuffer.getHeader(&attrCatHeader);

      for (int j = 0; j < attrCatHeader.numEntries; j++) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, j);

        if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) {
          const char *attrType = (attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER) ? "NUM" : "STR";
          printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
        }
      }
      attrBlockNum = attrCatHeader.rblock;
    }
    printf("\n");
  }
}

int main(int argc, char *argv[]) {
  Disk disk_run;

  printf("--- BEFORE UPDATE ---\n");
  printRelations();

  int attrBlockNum = ATTRCAT_BLOCK;
  bool updated = false;

  while (attrBlockNum != -1 && !updated) {
    RecBuffer attrCatBuffer(attrBlockNum);
    HeadInfo attrCatHeader;
    attrCatBuffer.getHeader(&attrCatHeader);

    for (int j = 0; j < attrCatHeader.numEntries; j++) {
      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
      attrCatBuffer.getRecord(attrCatRecord, j);

    
      if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, "Students") == 0 &&
          strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Class") == 0) {
        
        // Update the attribute name to "Batch"
        strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, "Batch");

        // Write the modified record back into the block buffer
        attrCatBuffer.setRecord(attrCatRecord, j);
        
        printf("Successfully updated 'Class' to 'Batch' for relation 'Students'.\n\n");
        updated = true;
        break;
      }
    }
    attrBlockNum = attrCatHeader.rblock;
  }

  if (!updated) {
    printf("Attribute 'Class' in relation 'Students' was not found.\n\n");
  }

  printf("--- AFTER UPDATE ---\n");
  printRelations();

  return 0;
}
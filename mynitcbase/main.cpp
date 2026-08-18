#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include<iostream>
// int main(int argc, char *argv[]) {
//   /* Initialize the Run Copy of Disk */
//   Disk disk_run;
//   // StaticBuffer buffer;
//   // OpenRelTable cache;

//   return FrontendInterface::handleFrontend(argc, argv);
// }

#include <cstdio>
#include <cstring>


int main(int argc, char *argv[]) {
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;

  // create objects for the relation catalog and attribute catalog
    RecBuffer relCatBuffer(RELCAT_BLOCK);
  HeadInfo relCatHeader;
  relCatBuffer.getHeader(&relCatHeader);

  // Iterate over each relation entry in the relation catalog
  for (int i = 0; i < relCatHeader.numEntries; i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    // Start at the first attribute catalog block
    int attrBlockNum = ATTRCAT_BLOCK;

    // Outer linked-list loop: traverse through all attribute catalog blocks
    while (attrBlockNum != -1) {
      RecBuffer attrCatBuffer(attrBlockNum);
      HeadInfo attrCatHeader;
      attrCatBuffer.getHeader(&attrCatHeader);

      // Inner loop: iterate over records in current attribute catalog block
      for (int j = 0; j < attrCatHeader.numEntries; j++) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, j);

        // Check if attribute belongs to the current relation
        if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) {
          const char *attrType = (attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER) ? "NUM" : "STR";
          printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
        }
      }

      // Move to the next block pointer in the linked list
      attrBlockNum = attrCatHeader.rblock;
    }

    printf("\n");
  }


  return 0;
}
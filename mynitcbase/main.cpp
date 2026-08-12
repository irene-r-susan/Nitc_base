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

  // create objects for the relation catalog and attribute catalog
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;

  // load the headers of both the blocks into relCatHeader and attrCatHeader.
  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);

  // Iterate over all relation catalog records using numEntries in header
  for (int i = 0; i < relCatHeader.numEntries; i++) {

    Attribute relCatRecord[RELCAT_NO_ATTRS]; // will store the record from the relation catalog

    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    // Iterate over all attribute catalog records using numEntries in header
    for (int j = 0; j < attrCatHeader.numEntries; j++) {

      // declare attrCatRecord and load the attribute catalog entry into it
      Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
      attrCatBuffer.getRecord(attrCatRecord, j);

      // Check if the attribute record corresponds to the current relation
      if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) {
        const char *attrType = (attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER) ? "NUM" : "STR";
        printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
      }
    }
    printf("\n");
  }

  return 0;
}
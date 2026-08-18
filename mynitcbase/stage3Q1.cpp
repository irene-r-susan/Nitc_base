#include "Cache/OpenRelTable.h"
#include "Buffer/StaticBuffer.h"
#include "Buffer/BlockBuffer.h"
#include <stdio.h>
#include <cstdio>
#include <cstring>

// linear-scan the relation catalog for a relation by name.
// fills relCatRecord if found.
int main(int argc,char*argv[]){
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;
   
  RelCatEntry relEntry;
  AttrCatEntry attrEntry;

  //RELCAT_RELid = 0 ATTRCAT_RELID = 1;
  for(int i=0;i<3;i++){
    RelCacheTable::getRelCatEntry(i,&relEntry);
    printf("Relation: %s\n",relEntry.relName);

    for(int j=0;j<relEntry.numAttrs;j++){    // instead of 5 i gave relEntry.numAttr cuzue student has only 4 so if 5 given btach printed twice
      AttrCacheTable::getAttrCatEntry(i,j,&attrEntry);
      printf(" %s %s\n",attrEntry.attrName,(attrEntry.attrType==NUMBER)?"NUM":"STR");
    }
    printf("\n");
  }
  // for ex we add student table as well so we give i<3
  return 0;
}

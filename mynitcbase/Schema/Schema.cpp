#include "Schema.h"

#include <cmath>
#include <cstring>

int Schema::openRel(char relName[ATTR_SIZE]) {
  int ret = OpenRelTable::openRel(relName);

  // the OpenRelTable::openRel() function returns the rel-id if successful
  // a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
  // error codes will be negative
  if(ret >= 0){
    return SUCCESS;
  }

  //otherwise it returns an error message
  return ret;
}

int Schema::closeRel(char relName[ATTR_SIZE]) {
  if (!strcmp(relName,RELCAT_RELNAME)||!strcmp(relName,ATTRCAT_RELNAME)) {
    return E_NOTPERMITTED;
  }

  // this function returns the rel-id of a relation if it is open or
  // E_RELNOTOPEN if it is not. we will implement this later.
  int relId = OpenRelTable::getRelId(relName);

  if (relId== E_RELNOTOPEN) {
    return E_RELNOTOPEN;
  }

  return OpenRelTable::closeRel(relId);
}

int Schema::renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]) {
    // 1. If oldRelName or newRelName is either Relation Catalog or Attribute Catalog, return E_NOTPERMITTED
    if (strcmp(oldRelName, RELCAT_RELNAME) == 0 || strcmp(oldRelName, ATTRCAT_RELNAME) == 0 ||
        strcmp(newRelName, RELCAT_RELNAME) == 0 || strcmp(newRelName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    // 2. If the relation is currently open, return E_RELOPEN
    // OpenRelTable::getRelId() returns E_RELNOTOPEN if the relation is NOT open
    int relId = OpenRelTable::getRelId(oldRelName);
    if (relId != E_RELNOTOPEN) {
        return E_RELOPEN;
    }

    // 3. Call BlockAccess::renameRelation to perform the actual update in the catalogs
    int retVal = BlockAccess::renameRelation(oldRelName, newRelName);
    return retVal;
}

int Schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName) {
    // 1. If relName is either Relation Catalog or Attribute Catalog, return E_NOTPERMITTED
    if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0) {
        return E_NOTPERMITTED;
    }

    // 2. If the relation is currently open, return E_RELOPEN
    // OpenRelTable::getRelId() returns E_RELNOTOPEN if the relation is NOT open
    int relId = OpenRelTable::getRelId(relName);
    if (relId != E_RELNOTOPEN) {
        return E_RELOPEN;
    }

    // 3. Call BlockAccess::renameAttribute to update the attribute name in Attribute Catalog
    int retVal = BlockAccess::renameAttribute(relName, oldAttrName, newAttrName);
    return retVal;
}
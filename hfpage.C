#include <iostream>
#include <stdlib.h>
#include <memory.h>

#include "hfpage.h"
#include "buf.h"
#include "db.h"


// **********************************************************
// page class constructor
void HFPage::init(PageId pageNo)
{
    // slot count intially is 0
    this->slotCnt = 0;
    // free space
    this->freeSpace = MAX_SPACE - DPFIXED + sizeof(slot_t);
    // used ptr
    this->usedPtr = MAX_SPACE - DPFIXED;
    // set page numbers
    this->curPage = pageNo;
    this->prevPage = INVALID_PAGE;
    this->nextPage = INVALID_PAGE;
    // intial slot
    this->slot[0].offset = 0;
    this->slot[0].length = EMPTY_SLOT;
}
// **********************************************************
// dump page utlity
void HFPage::dumpPage()
{
    int i;

    cout << "dumpPage, this: " << this << endl;
    cout << "curPage= " << curPage << ", nextPage=" << nextPage << endl;
    cout << "usedPtr=" << usedPtr << ",  freeSpace=" << freeSpace
         << ", slotCnt=" << slotCnt << endl;
   
    for (i=0; i < slotCnt; i++) {
        cout << "slot["<< i <<"].offset=" << slot[i].offset
             << ", slot["<< i << "].length=" << slot[i].length << endl;
    }
}
// **********************************************************
PageId HFPage::getPrevPage()
{
    // get the previous page number
    return this->prevPage;
}
// **********************************************************
void HFPage::setPrevPage(PageId pageNo)
{
    // set previous page number 
    this->prevPage = pageNo;
}
// **********************************************************
PageId HFPage::getNextPage()
{
    // get next page number
    return this->nextPage;
}
// **********************************************************
void HFPage::setNextPage(PageId pageNo)
{
    // set next page number
    this->nextPage = pageNo;
}
// **********************************************************
// Add a new record to the page. Returns OK if everything went OK
// otherwise, returns DONE if sufficient space does not exist
// RID of the new record is returned via rid parameter.
Status HFPage::insertRecord(char* recPtr, int recLen, RID& rid)
{
    if (available_space() < recLen) return DONE;
    int slotNum = 0;
    //Find an empty slot
    for (slotNum = 0; slotNum < slotCnt; slotNum++) {
        if (slot[slotNum].length == EMPTY_SLOT) break;
    }
    //move the used pointer back from the end
    this->usedPtr = this->usedPtr - recLen;
    //copy the parameters into the found slot
    slot[slotNum].offset = this->usedPtr;
    slot[slotNum].length = recLen;
    //copy the record into the page
    memcpy(data + usedPtr, recPtr, recLen);
    //adjust page values to reflect new record
    this->freeSpace -= recLen;
    if (slotNum == this->slotCnt) slotCnt++;
    //update record values based on the page
    rid.slotNo = slotNum;
    rid.pageNo = this->curPage;
    return OK;
}

// **********************************************************
// Delete a record from a page. Returns OK if everything went okay.
// Compacts remaining records but leaves a hole in the slot array.
// Use memmove() rather than memcpy() as space may overlap.
Status HFPage::deleteRecord(const RID& rid)
{
    // perform sanity checks
    if (slotCnt == 0 || rid.slotNo < 0 ||
        rid.slotNo > slotCnt || slot[rid.slotNo].length == EMPTY_SLOT ||
        rid.pageNo != curPage
       )
        return FAIL;
    int recLen = slot[rid.slotNo].length;
    int recOffset = slot[rid.slotNo].offset;

    //Move all lower records 1 closer to the end of the page, covering this record
    memmove(data + this->usedPtr + recLen, data + this->usedPtr, recFffset - this->usedPtr);
    //Adjust usedPtr to reflect new edge of records
    this->usedPtr += recLen;
    this->freeSpace += recLen;

    slot[rid.slotNo].length = EMPTY_SLOT;
    //Clear out unused slots at the end of the slot array
    while(slot[slotCnt - 1].length != EMPTY_SLOT) slotCnt--;

    //Adjust the offsets of the other records
    if (rid.slotNo<slotCnt) {
        for (int i=rid.slotNo + 1; i < slotCnt; i++) {
            slot[i].offset += recLen;
        }
    }

    return OK;
}

// **********************************************************
// returns RID of first record on page
Status HFPage::firstRecord(RID& firstRid)
{
    // perform sanity checks
    if (slotCnt == 0 || slot[0].length == EMPTY_SLOT)
        return FAIL;
    // pass data members
    firstRid.pageNo = curPage;
    firstRid.slotNo = 0;
    // return status
    return OK;
}

// **********************************************************
// returns RID of next record on the page
// returns DONE if no more records exist on the page; otherwise OK
Status HFPage::nextRecord (RID curRid, RID& nextRid)
{
    // perform sanity checks
    if (    slotCnt == 0 || curRid.slotNo < 0 ||
            curRid.slotNo > slotCnt || slot[curRid.slotNo].length == EMPTY_SLOT ||
            curRid.pageNo != curPage
       )
        return FAIL;
    // check if this is the last slot
    if (curRid.slotNo + 1 == slotCnt)
        return DONE;
    // pass data members
    nextRid.pageNo = curPage;
    nextRid.slotNo = curRid.slotNo + 1;
    // return status
    return OK;
}
// **********************************************************
// returns length and copies out record with RID rid
Status HFPage::getRecord(RID rid, char* recPtr, int& recLen)
{
    // perform sanity checks
    if (    slotCnt == 0 || rid.slotNo < 0 || 
            rid.slotNo >= slotCnt || slot[rid.slotNo].length == EMPTY_SLOT || 
            rid.pageNo != curPage
       ) 
        return FAIL;
    // copy out data
    memcpy(recPtr, data + slot[rid.slotNo].offset, recLen);
    // set record length
    recLen = slot[rid.slotNo].length;
    // return status
    return OK;
}
// **********************************************************
// returns length and pointer to record with RID rid.  The difference
// between this and getRecord is that getRecord copies out the record
// into recPtr, while this function returns a pointer to the record
// in recPtr.
Status HFPage::returnRecord(RID rid, char*& recPtr, int& recLen)
{
    // perform sanity checks
    if (slotCnt == 0 || rid.slotNo < 0 ||
        rid.slotNo >= slotCnt || slot[rid.slotNo].length == EMPTY_SLOT ||
        rid.pageNo != curPage
       )
        return FAIL;
    // pass data members
    recPtr = &data[slot[rid.slotNo].offset];
    recLen = slot[rid.slotNo].length;
    // return status
    return OK;
}
// **********************************************************
// Returns the amount of available space on the heap file page
int HFPage::available_space(void)
{
    // check if any slots exist yet
    if (!this->slotCnt)
        return this->freeSpace - sizeof(slot_t);
    // default
    return this->freeSpace - slotCnt * sizeof(slot_t);
}
// **********************************************************
// Returns 1 if the HFPage is empty, and 0 otherwise.
// It scans the slot directory looking for a non-empty slot.
bool HFPage::empty(void)
{
    // iterate over the array and return false if a slot is being used
    for (int i = 0; i < this->slotCnt; i++)
        if (this->slot[i].length != EMPTY_SLOT)
            return false;
    // default 
    return true;
}
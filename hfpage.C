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
    this->nextPage = INVALID_PAGE;
    this->prevPage = INVALID_PAGE;
    this->curPage = pageNo;
    this->slotCnt = 0;
    this->usedPtr = MAX_SPACE - DPFIXED;
    this->freeSpace = MAX_SPACE - DPFIXED + sizeof(slot_t);
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
    return this->prevPage;
}

// **********************************************************
void HFPage::setPrevPage(PageId pageNo)
{
    this->prevPage = pageNo;
}

// **********************************************************
PageId HFPage::getNextPage()
{
    return this->nextPage;
}

// **********************************************************
void HFPage::setNextPage(PageId pageNo)
{
    this->nextPage = pageNo;
}

// **********************************************************
// Add a new record to the page. Returns OK if everything went OK
// otherwise, returns DONE if sufficient space does not exist
// RID of the new record is returned via rid parameter.
Status HFPage::insertRecord(char* recPtr, int recLen, RID& rid)
{
    if(available_space() < recLen) return DONE;
    int slotNum = 0;
    //Find an empty slot
    for(slotNum=0;slotNum<slotCnt;slotNum++) {
	if(slot[slotNum].length == EMPTY_SLOT) break;
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
    if(slotNum == this->slotCnt) slotCont++;
    //update record values based on the page
    rid.slotNo = this->slotNum;
    rid.pageNo = this->curPage;
    return OK;
}

// **********************************************************
// Delete a record from a page. Returns OK if everything went okay.
// Compacts remaining records but leaves a hole in the slot array.
// Use memmove() rather than memcpy() as space may overlap.
Status HFPage::deleteRecord(const RID& rid)
{
    // fill in the body
    return OK;
}

// **********************************************************
// returns RID of first record on page
Status HFPage::firstRecord(RID& firstRid)
{
    // fill in the body
    return OK;
}

// **********************************************************
// returns RID of next record on the page
// returns DONE if no more records exist on the page; otherwise OK
Status HFPage::nextRecord (RID curRid, RID& nextRid)
{
    // fill in the body

    return OK;
}

// **********************************************************
// returns length and copies out record with RID rid
Status HFPage::getRecord(RID rid, char* recPtr, int& recLen)
{
    if(slotCnt == 0 || rid.slotNo < 0 || rid.slotNo >= slotCnt || slot[rid.slotNo].length == EMPTY_SLOT || rid.pageNo != curPage) return FAIL;
    return OK;
}

// **********************************************************
// returns length and pointer to record with RID rid.  The difference
// between this and getRecord is that getRecord copies out the record
// into recPtr, while this function returns a pointer to the record
// in recPtr.
Status HFPage::returnRecord(RID rid, char*& recPtr, int& recLen)
{
    // fill in the body
    return OK;
}

// **********************************************************
// Returns the amount of available space on the heap file page
int HFPage::available_space(void)
{
    if(!slotCnt) return this->freeSpace - sizeof(slot_t);
    return this->freeSpace - slotCnt * sizeof(slot_t);
}

// **********************************************************
// Returns 1 if the HFPage is empty, and 0 otherwise.
// It scans the slot directory looking for a non-empty slot.
bool HFPage::empty(void)
{
    for(int i=0;i<this->slotCnt;i++) {
	if(slot[i].length != EMPTY_SLOT) return false;
    }
    return true;
}




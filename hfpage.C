#include <iostream>
#include <stdlib.h>
#include <memory.h>

#include "hfpage.h"
#include "buf.h"
#include "db.h"

/*
    Authors: Thomas Ruff, Alex Erwin
    CSC 560 Project 1
    September 7, 2021
    File: hfpage.C - Implementation of a heap file page with insertion, deletion, and retrieval of records.
    Does not currently allow modification of records other than deletion/insertion.
    Contains a slot directory on the page which indicates which records are present at what offset.
    Open slots are allowed in the slot directory, but not at the end.
    Implemented in C++
*/

// **********************************************************
// page class constructor
void HFPage::init(PageId pageNo)
{
	//slot count is initially zero
	slotCnt=0;
	//used ptr
	usedPtr=MAX_SPACE - DPFIXED;
	//free space considers initial slot
	freeSpace=MAX_SPACE - DPFIXED+ sizeof(slot_t);
	//Hit initial page numbers
	prevPage=INVALID_PAGE;
	nextPage=INVALID_PAGE;
	curPage=pageNo;
	//Set up the first slot
	slot[0].offset=-1;
	slot[0].length=-1;
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
	int slotNum = slotCnt;
	int spaceNeeded = recLen+sizeof(slot_t);
	int slotFlag = 1;
	//Search for an open slot
	for (int i = 0 ; i < slotCnt; ++i) {
		if (slot[i].length==-1) {
			//When a slot is found, insert into that slot
			spaceNeeded = recLen;
			slotNum = i;
			//No new slot is needed
			slotFlag = 0;
			break;
		}
	}

	if (spaceNeeded > this->available_space()+4)
		return DONE;

	//Adjust data array
	slot[slotNum].offset=usedPtr-recLen;
	slot[slotNum].length=recLen;
	for (int i =0 ; i != recLen; ++i)
	{
		data[usedPtr-recLen+i]=recPtr[i];
	}
	//update member parameters;
	usedPtr = usedPtr - recLen;
	freeSpace -= spaceNeeded;
	rid.pageNo = curPage;
	rid.slotNo = slotNum;
	if(slotFlag == 1) {
		slotCnt += 1;
	}
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
    memmove(data + this->usedPtr + recLen, data + this->usedPtr, recOffset - this->usedPtr);
    //Adjust usedPtr to reflect new edge of records
    this->usedPtr += recLen;
    this->freeSpace += recLen;

    slot[rid.slotNo].length = EMPTY_SLOT;
    //Clear out unused slots at the end of the slot array
    for(int i=slotCnt-1;i>-1;i--) {
	if(slot[i].length != EMPTY_SLOT) break;
	slotCnt--;
    }

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
    if(empty()) return DONE;
    if (slotCnt == 0)
        return FAIL;
    // Find the first slot filled
    for (int i = 0; i < slotCnt; i++) {
        if (slot[i].length != EMPTY_SLOT) {
            firstRid.slotNo = i;
            firstRid.pageNo = curPage;
            break;
        }
    }
    // return status
    return OK;
}
// **********************************************************
// returns RID of next record on the page
// returns DONE if no more records exist on the page; otherwise OK
Status HFPage::nextRecord (RID curRid, RID& nextRid)
{
    // perform sanity checks
    if(empty() || curRid.pageNo != curPage) return FAIL;
    // check if this is the last slot
    if (curRid.slotNo + 1 == slotCnt)
        return DONE;
    // pass data members
    for(int i=curRid.slotNo+1;i<slotCnt;i++) {
	if(slot[i].length != EMPTY_SLOT) {
	    nextRid.slotNo = i;
	    nextRid.pageNo = curPage;
	    return OK;
	}
    }
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
    for(int i=0;i<slot[rid.slotNo].length;i++) {
	*recPtr = data[slot[rid.slotNo].offset+i];
	recPtr++;
    }
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
    recPtr = &data[this->slot[rid.slotNo].offset];
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
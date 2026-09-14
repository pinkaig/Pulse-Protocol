/******************************************************************************/
/**
 * @file        ObjectAllocator.cpp
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       TBF
 *
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

#include "ObjectAllocator.h"
/*
Page size should be at least 4096 bytes, in multiples of this
Create test project with struct Student. And see if can replicate
the diagram. Address of the block is by calling operator new
*/
//ObjectAllocator should set OAstats OA config and construct the freelist and pagelist right
//OAconfig must be made BEFORE ObjectAllocated!!!
ObjectAllocator::ObjectAllocator(unsigned ObjectSize, OAConfig const& config)
:Config_(config),OAStats_()/*explicit default construction*/
, freeList_(nullptr), pageList_(nullptr) {
    //set stats, compute page size , allocate first page
    //set stats first
    //OAStats_ = OAStats();
    OAStats_.ObjectSize_ = ObjectSize;
    //set basepagesize, why not sizet?
    //const unsigned pageBytes = 4096; // could be 8192, 12288 multiples of 2
    // size of a page: ObjectsPerPage_ * ObjectSize_ + sizeof(void*)
    // [page header ptr][ObjectsPerPage blocks]
    //sizeof(GenericObject*) = 8 in 64bit
    //sizeof(GenericObject*) = 4 in 32bit;
    //sizeof(Student) == 32 in 64bit system
    //sizeof(Student) == 16 in 32bit system
     OAStats_.PageSize_ =  Config_.ObjectsPerPage_ * OAStats_.ObjectSize_ 
     + static_cast<unsigned>(sizeof(GenericObject));

    // Allocate first page now
    unsigned char* rawPage = nullptr;
    try
    {
        rawPage = reinterpret_cast<unsigned char*>(::operator new(OAStats_.PageSize_));
    }
    catch (...) //catch any expcetion regardless of type, since explicitly said to be OAException
    {
        throw OAException(OAException::E_NO_MEMORY, "[ObjectAllocator] ctor: cannot allocate first page");
    }
    
    // Put page on page list: store "Next" pointer in first bytes of the page
    GenericObject* pageNode = reinterpret_cast<GenericObject*>(rawPage);
    pageNode->Next = pageList_;
    pageList_ = pageNode;

  // Blocks start immediately after the page header
    unsigned char* firstBlock = rawPage + sizeof(GenericObject);

    // Build free list by treating each block as a GenericObject node
    // Use LIFO so the "rightmost" block is handed out first shown in diagram
    //[ page header ][ block 0 ][ block 1 ][ block 2 ][ block 3 ]
    // each block = node in the free-list
    // chain together using Next ptr (singly-linked list)
    //Most recently freed block would be used first to allocate!!!
    for (unsigned i = 0; i < Config_.ObjectsPerPage_; ++i)
    {
        //byte arithmetic //first block points to [block 0].. etc 
        unsigned char* blockAddr = firstBlock + i * OAStats_.ObjectSize_;
        //reinterpret the block as free-list node
        GenericObject* blockNode = reinterpret_cast<GenericObject*>(blockAddr);
        //assign to freelist ptr, push block onto freelist
        blockNode->Next = freeList_; 
        freeList_ = blockNode;
    }

    // Update stats to reflect one ready page
    OAStats_.PagesInUse_ = 1;
    OAStats_.FreeObjects_ = Config_.ObjectsPerPage_;
    OAStats_.ObjectsInUse_ = 0;
    OAStats_.MostObjects_ = 0;
    OAStats_.Allocations_ = 0;
    OAStats_.Deallocations_ = 0;

    std::cout << "[ObjectAllocator] Constructed!!!\n";
}// end of default ctor


ObjectAllocator::~ObjectAllocator(){//} throw(){
    // Walk the page list and free each page.
    // Each page begins with a GenericObject header whose Next links to the next page.
    // as long as page list is not a nullptr(not empty)
    // delete the current page and then assign current page to be the next ptr
    GenericObject* page = pageList_;
    while (page ) 
    {
        GenericObject* next = page->Next;      // save next page before freeing this one
        ::operator delete(page );               // free raw page memory (must match ::operator new)
        page  = next;
    }

    // explicitly assigned clean up, assigned nullptr
    pageList_ = nullptr;
    freeList_ = nullptr;
}// end of dtor

//if Config_.UseCPPMemManager_ is true, use ::operator new(ObjectSize_)
// if freelist_ == nullptr, allocate a new page (unless MaxPages_ reached),
// link it into pageList_ and build its blocks onto freeList_
// pophead node from freeList_
// update states Allocations_++, FreeObjects_--, ObjectsInUse++
// update mostobjects_?
/*
debug mode:
When a new page is created (and debug mode is ON), it fills every block in that page with a known byte value
"UNALLOCATED_PATTERN" = 0xaa
[AA AA AA AA ...]  block 0
[AA AA AA AA ...]  block 1
[AA AA AA AA ...]  block 2
*/
void* ObjectAllocator::Allocate() /*throw(OAException)*/ {
    //this means use default CPP new/delete. if CPPmemManager is true
    if (Config_.UseCPPMemManager_) {
        //call global function operator new with ObjectSize_ bytes
        //assigned to raw pointer, p
        void* p = ::operator new(OAStats_.ObjectSize_);
        OAStats_.Allocations_++;
        OAStats_.ObjectsInUse_++;
        //update MostObjects(The highest number of objects that 
        //was simultaneously allocated at any point in the program's lifetime.)
        if (OAStats_.ObjectsInUse_ > OAStats_.MostObjects_) {
            OAStats_.MostObjects_ = OAStats_.ObjectsInUse_;
        }
        return p;
    }// end of block of using default CPP 


     // freeList is nullptr means there are ZERO blocks of memory left
     //so we need to allocate new pages if max number of pages is not reached
    if (!freeList_) {
        //max pages is assigned zero and pagesinuse >= throw exception
        if (Config_.MaxPages_ != 0 && OAStats_.PagesInUse_ >= Config_.MaxPages_) {
            throw OAException(OAException::E_NO_PAGES,
                "[ObjectAllocator] Allocate : max pages reached or maxpages set to zero ");
        }

        unsigned char* rawPage = nullptr;
        try {
            rawPage = reinterpret_cast<unsigned char*>(::operator new(OAStats_.PageSize_));
        }
        catch (...) {
            throw OAException(OAException::E_NO_MEMORY, "[ObjectAllocator] Allocate: cannot allocate new page");
        }

        // link page into page list(Head of singly-linked list of pages)
        /*
        Each page:
        1) lives in heap mem
        2) uses its first sizeof(GenericObject) bytes as a "node"
        3) Stores Next pointer to the previous page(for page/node traversal)
        - Every page is both a chunk of memory and a node in the page list

        firstpage
            pageList_ -> pageA -> nullptr
        secondpage
            pageList_ -> pageB -> pageA -> nullptr
        thirdpage
            pageList_ -> pageC -> pageB -> pageA -> nullptr
        pageList always point to the most recently allocated page
        older pages can be reached using "Next" variable
        */
        //need to reinterpret_cast since we are dealing with raw bytes
        //pageNode IS the page header
        GenericObject* pageNode = reinterpret_cast<GenericObject*>(rawPage);
        pageNode->Next = pageList_; //assign Next to previous page
        pageList_ = pageNode; //assign pagelist to new page

        // Build free-list from blocks in the page
        /*
        1) Page memory is contiguous and partitioend into fixed-size blocks (ObjectSize_ bytes).
        2) Allocation may appear "backwards" because blocks are pushed to the free list
        in LIFO(last in first out) order.
        3) GenericObject is used to overlay a 'Next' pointer on top of the block while it is free.
        */
        unsigned char* firstBlock = rawPage + sizeof(GenericObject);

        // DEBUG MODE (optional): mark every block as "unallocated" first
        if (Config_.DebugOn_) {
            for (unsigned i = 0; i < Config_.ObjectsPerPage_; ++i) {
                unsigned char* blockAddr = firstBlock + i * OAStats_.ObjectSize_;
                for (unsigned j = 0; j < OAStats_.ObjectSize_; ++j) {
                    blockAddr[j] = UNALLOCATED_PATTERN;
                }
            }
        }

        for (unsigned i = 0; i < Config_.ObjectsPerPage_; ++i) {
            // Compute address of block i: base + i * block size
            unsigned char* blockAddr = firstBlock + i * OAStats_.ObjectSize_;

            //Treat free block memory as a free-list node
            //use first few bytes of "memory" to store a ptr while block is free
            // "pretend a memory starting at blockAddr is a GO"
            // so we can write next ptor there

            //overwrites the first sizeof(void*) bytes of each block, because that�s where blockNode->Next lives.
            //[ Next pointer bytes ][ AA AA AA AA AA ... ]
            GenericObject* blockNode = reinterpret_cast<GenericObject*>(blockAddr);

            // (linking blockNode to freelist), Push block onto free list 
            // same as a stack LIFO
            blockNode->Next = freeList_; //asign Next to previous block of mem
            freeList_ = blockNode;  //assign freelist to new block
        }

        //increment stats
        OAStats_.PagesInUse_++;
        OAStats_.FreeObjects_ += Config_.ObjectsPerPage_;
    }// end of assigning new pages if no freelist

    if (!freeList_) {
        throw OAException(OAException::E_NO_MEMORY, "[ObjectAllocator] no free page");
    }

    //Actual Allocation
    // pop from free list, typical when there is space on the freelist
    GenericObject* node = freeList_;  // take the current head (the block we will return)
    // "pop" by assgining current ptor to next block
    freeList_ = freeList_->Next; // advance head to the next free block

    //update stats
    OAStats_.Allocations_++;
    OAStats_.FreeObjects_--;
    OAStats_.ObjectsInUse_++;
    //update MostObjects(The highest number of objects that 
    //was simultaneously allocated at any point in the program's lifetime.)
    if (OAStats_.ObjectsInUse_ > OAStats_.MostObjects_) {
        OAStats_.MostObjects_ = OAStats_.ObjectsInUse_;
    }

    // DEBUG MODE: mark the block we are handing out as "allocated"
    //block no longer free, mark the whole block as 0xBB
    //if accessing bad memory e.g. unintiializaed memory
    //Student* s = (Student*)node;
    //std::cout << s->Age; // reads 0xBBBBBBBB
    if (Config_.DebugOn_) {
        unsigned char* bytes = reinterpret_cast<unsigned char*>(node);
        for (unsigned i = 0; i < OAStats_.ObjectSize_; ++i) {
            bytes[i] = ALLOCATED_PATTERN;
        }
    }

    return node; //return allocated block of memory
}
// If UseCPPMemManager_ , is true use ::operator delete(Object) and update stats.
// Otherwise validate (later: boundary checks / multiple free / pad bytes if DebugOn).
// Push block back onto freeList_
// Update stats: Deallocations_++, FreeObjects_++, ObjectsInUse_--.
// 
/*
Allocator version of "delete"
1) hands the pointer back to the normal heap
2) puts the block back onto the OA free list
*/
void ObjectAllocator::Free(void* Object) /*throw(OAException)*/ {

    if (!Object) { return; }

    //using CPP delete instead, dont use pool use real heap
    if (Config_.UseCPPMemManager_) {
        ::operator delete(Object);
        OAStats_.Deallocations_++;
        OAStats_.ObjectsInUse_--;
        return;
    }

    /// FASTer : Debug OFF
    if (!Config_.DebugOn_) {
        GenericObject* node = reinterpret_cast<GenericObject*>(Object);
        node->Next = freeList_;
        freeList_ = node;

        OAStats_.Deallocations_++;
        OAStats_.FreeObjects_++;
        OAStats_.ObjectsInUse_--;
        return;
    }
    // DEBUG PATH: Debug ON
    else {
        // (later) validate boundary/multiple-free/pad bytes here
           // 1) Pointer not from this allocator
        unsigned char* obj = reinterpret_cast<unsigned char*>(Object);

        // 1) Pointer not from this allocator (E_BAD_ADDRESS)
        GenericObject* owningPage = nullptr;
        //iterate through page list
        for (GenericObject* p = pageList_; p; p = p->Next)
        {
            unsigned char* pageStart = reinterpret_cast<unsigned char*>(p);
            //start+total size
            unsigned char* pageEnd = pageStart + OAStats_.PageSize_;

            if (obj >= pageStart && obj < pageEnd) //if inside page
            {
                owningPage = p; // assign owning page to p
                break;//break out of loop
            }
        }

        if (!owningPage) {  //no owning page
            throw OAException(OAException::E_BAD_ADDRESS, "[ObjectAllocator] Free: pointer not in any page");
        }

        // 2) Pointer is inside a page but not at start of a block
        //is this pointer EXACTLY inside the start block
        unsigned char* pageStart = reinterpret_cast<unsigned char*>(owningPage);
        unsigned char* firstBlock = pageStart + sizeof(GenericObject);

        // must be within the block region
        unsigned char* blockRegionEnd = firstBlock + Config_.ObjectsPerPage_ * OAStats_.ObjectSize_;
        if (obj < firstBlock || obj >= blockRegionEnd) {
            throw OAException(OAException::E_BAD_BOUNDARY, "[ObjectAllocator] Free: pointer not in block region");
        }

        unsigned offset = static_cast<unsigned>(obj - firstBlock);
        if (offset % OAStats_.ObjectSize_ != 0) {
            throw OAException(OAException::E_BAD_BOUNDARY, "[ObjectAllocator] Free: pointer not on block boundary");
        }

        // 3) Pointer already Freed
        for (GenericObject* n = freeList_; n; n = n->Next)
        {
            if (reinterpret_cast<void*>(n) == Object) { //block is already freed found on the free list
                throw OAException(OAException::E_MULTIPLE_FREE, "[ObjectAllocator] Free: double free detected");
            }

        }


        // 4) Memory overwritten Only possible when implement PadBytes_ (and/or headers).


        //pool mode, using our memory pool, give it back to the freelist
        //block of memory we are returning to the free pool
        GenericObject* node = reinterpret_cast<GenericObject*>(Object);
        //block that used to be at top of stack becomes next block after node
        node->Next = freeList_;
        //now this node is at the top of free stack
        freeList_ = node;

        //update stats
        OAStats_.Deallocations_++;
        OAStats_.FreeObjects_++;
        OAStats_.ObjectsInUse_--;
    }

}

//Gettters and Setters
void ObjectAllocator::SetDebugState(bool State){
    Config_.DebugOn_ = State;
}
const void *ObjectAllocator::GetFreeList() const{// returns a pointer to the internal free list
    return freeList_;
}
const void *ObjectAllocator::GetPageList() const{ // returns a pointer to the internal page list
    return pageList_;
}

//return by value, not modifying anything
OAConfig ObjectAllocator::GetConfig() const{       // returns the configuration parameters
    return Config_;
}
OAStats ObjectAllocator::GetStats() const{    // returns the statistics for the allocator
    return OAStats_;
}

/*
OA_EXCEPTION -> custom exception type
You can catch OAException if (and only if) 
some code executed inside the try block throws an OAException (directly or indirectly).

//correct since   void *Allocate() throw(OAException);
try {
    void* p = allocator.Allocate();
}
catch (const OAException& e) {
    std::cerr << "OAException caught!\n";
    std::cerr << "Error code: " << e.code() << "\n";
    std::cerr << "Message: " << e.what() << "\n";
}


reinterpret_cast in C++ is a low-level casting operator 
that tells the compiler to treat a memory address as a completely different data type
bit-level interprtation
*/



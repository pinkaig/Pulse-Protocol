/******************************************************************************/
/**
 * @file        ObjectAllocator.h
 * @project     Pulse Protocol
 * @author      Ban Kai Wei Benjamin
 * @brief       Fixed-size memory pool allocator that serves objects from pages 
 *              of pre-allocated blocks. integrated with std::unique_ptr via a custom deleter.
 *             
 *              
 * 
 * @copyright   Copyright (C) 2026 DigiPen Institute of Technology.
 *              Reproduction or disclosure of this file or its contents without the
 *              prior written consent of DigiPen Institute of Technology is prohibited.
 */
 /******************************************************************************/

//---------------------------------------------------------------------------
#ifndef OBJECTALLOCATORH
#define OBJECTALLOCATORH
//---------------------------------------------------------------------------

#ifdef _MSC_VER
#pragma warning( disable : 4290 ) // suppress warning: C++ Exception Specification ignored
#endif
#include "CoreEngine/Core/ImportExport.h"
#include <string>
#include <iostream>
#include <memory>
// If the client doesn't specify these:
inline constexpr int DEFAULT_OBJECTS_PER_PAGE = 4;
inline constexpr int DEFAULT_MAX_PAGES = 3;
/*====================================================================================================================*/
// ObjectAllocator configuration parameters
class OAException
{
  public:
      // Possible exception codes
    enum OA_EXCEPTION 
    {
      E_NO_MEMORY,      // out of physical memory (operator new fails)
      E_NO_PAGES,       // out of logical memory (max pages has been reached)
      E_BAD_ADDRESS,    // block address is not on a page
      E_BAD_BOUNDARY,   // block address is on a page, but not on any block-boundary
      E_MULTIPLE_FREE,  // block has already been freed
      E_CORRUPTED_BLOCK // block has been corrupted (pad bytes have been overwritten)
    };

    OAException(OA_EXCEPTION ErrCode, const std::string& Message) : error_code_(ErrCode), message_(Message) {};

    virtual ~OAException() {
    }

    OA_EXCEPTION code(void) const { 
      return error_code_; 
    }

    virtual const char *what(void) const {
      return message_.c_str();
    }
  private:  
    OA_EXCEPTION error_code_;
    std::string message_;
};
/*====================================================================================================================*/
// ObjectAllocator configuration parameters
// policy/config. Tells allocator
// 1. whether to use custom pool allocator or just new/delete
// 2. how many objs per page and max no. of pages.
// 3. enable debug checks/pattern fills
// 4. layout each block (pad bytes, header bytes, alignment bytes)
struct OAConfig
{
  OAConfig(bool UseCPPMemManager = false, 
           unsigned ObjectsPerPage = DEFAULT_OBJECTS_PER_PAGE, 
           unsigned MaxPages = DEFAULT_MAX_PAGES, 
           bool DebugOn = false, 
           unsigned PadBytes = 0,
           unsigned HeaderBlocks = 0,
           unsigned Alignment = 0) : UseCPPMemManager_(UseCPPMemManager), 
                                     ObjectsPerPage_(ObjectsPerPage), 
                                     MaxPages_(MaxPages), 
                                     DebugOn_(DebugOn), 
                                     PadBytes_(PadBytes),
                                     HeaderBlocks_(HeaderBlocks),
                                     Alignment_(Alignment)
  {
      std::cout << "[OAConfig] Constructed\n";
    LeftAlignSize_ = 0;
    InterAlignSize_ = 0;
  }

  //if true use c++ normal alloc , if false use ours
  bool UseCPPMemManager_;   // by-pass the functionality of the OA and use new/delete
  
    //needed to know to 
    // 1. Build a page
    // 2. Compute page size
    // 3. Allocate new pages inside Allocate()
    // 4. Free empty pages later on
    //pag size depends on this variable
  unsigned ObjectsPerPage_; // number of objects on each page
  //max number of pages this allocator may allocate
  unsigned MaxPages_;       // maximum number of pages the OA can allocate (0=unlimited)
  bool DebugOn_;            // enable/disable debugging code (signatures, checks, etc.)
  
  //[header][left pad][OBJECT][right pad][(alignment gap)]
  unsigned PadBytes_;       // size of the left/right padding for each block
  unsigned HeaderBlocks_;   // size of the header for each block (0=no headers)
  unsigned Alignment_;      // address alignment of each block

  //how many wasted byte to insert before the first block on a page so that first block is
  //aligned properly
  unsigned LeftAlignSize_;  // number of alignment bytes required to align first block
  //insert between each block so each block start is aligned
  unsigned InterAlignSize_; // number of alignment bytes required between remaining blocks
};
/*====================================================================================================================*/
// ObjectAllocator configuration parameters
// ObjectAllocator statistical info
// runtime accounting for allocator, monitors internal allocation
//Track
//memory usage
// alloc/dealloc
//page usage
//high-water marks(maximum level of resource usage reached at any point in time)
//“What was the highest amount of this resource I ever used?”
struct OAStats
{
  OAStats(void) : ObjectSize_(0), FreeObjects_(0), ObjectsInUse_(0), PagesInUse_(0),
                  PageSize_(0), MostObjects_(0), Allocations_(0), Deallocations_(0) {
      std::cout << "[OAStats] Constructed\n";
  };

  unsigned ObjectSize_;    // size of each object
  unsigned FreeObjects_;   // number of objects on the free list
  unsigned ObjectsInUse_;  // number of objects in use by client
  unsigned PagesInUse_;    // number of pages allocated
  unsigned PageSize_;      // size of a page: ObjectsPerPage_ * ObjectSize_ + sizeof(void*)
  //most objects allocated in memory at current point
  //for capacity planning
  //detecing memory spikes
  unsigned MostObjects_;   // most objects in use by client at one time
  unsigned Allocations_;   // total requests to allocate memory
  unsigned Deallocations_; // total requests to free memory
};

// This allows us to easily treat raw objects as nodes in a linked list
struct GenericObject
{
  GenericObject *Next; //ptor to next node
};

// This memory manager class 
class DLL_API ObjectAllocator
{
  public:
    typedef void (*DUMPCALLBACK)(const void *, unsigned int);
    typedef void (*VALIDATECALLBACK)(const void *, unsigned int);

      // Predefined values for memory signatures
    static const unsigned char UNALLOCATED_PATTERN = 0xaa;
    static const unsigned char ALLOCATED_PATTERN = 0xbb;
    static const unsigned char FREED_PATTERN = 0xcc;
    static const unsigned char PAD_PATTERN = 0xdd;
    static const unsigned char ALIGN_PATTERN = 0xee;

      // Creates the ObjectManager per the specified values
      // Throws an exception if the construction fails. (Memory allocation problem)
    ObjectAllocator(unsigned ObjectSize, const OAConfig& config);// throw(OAException);

      // Destroys the ObjectManager (never throws)
    ~ObjectAllocator(); //throw();

    //   // Take an object from the free list and give it to the client (simulates new)
    //   // Throws an exception if the object can't be allocated. (Memory allocation problem)
    // void *Allocate() throw(OAException);

    //   // Returns an object to the free list for the client (simulates delete)
    //   // Throws an exception if the the object can't be freed. (Invalid object)
    // void Free(void *Object) throw(OAException);


void *Allocate();                                             // can still throw
void Free(void *Object);                                      // can still throw

    //  // Calls the callback fn for each block still in use
    // //Use it when you want to print leak
    //      -at program shutdown
    //      -after a test case finishes
    //unsigned DumpMemoryInUse(DUMPCALLBACK fn) const;

    //  // Calls the callback fn for each block that is potentially corrupted
    //  // Use it in debug mode to catch “bad memory writes” early
    //  // after Allocate(), Free() , or in a debug command / test
    //  
    //unsigned ValidatePages(VALIDATECALLBACK fn) const;

    //  // Frees all empty pages (give memory back)
    // // Use after a bunch of frees (like level unload)
    // // or stress tests
    //unsigned FreeEmptyPages(void);
    

    //  // [EXTRA] Returns true if FreeEmptyPages and alignments are implemented
    //static bool ImplementedExtraCredit(void);

      // Testing/Debugging/Statistic methods
    void SetDebugState(bool State);       // true=enable, false=disable
    const void *GetFreeList(void) const;  // returns a pointer to the internal free list
    const void *GetPageList(void) const;  // returns a pointer to the internal page list
    OAConfig GetConfig(void) const;       // returns the configuration parameters
    OAStats GetStats(void) const;         // returns the statistics for the allocator

    

  private:
    OAConfig Config_;            // configuration parameters
    OAStats OAStats_;            // accumulating statistics

      // Make private to prevent copy construction and assignment
    ObjectAllocator(const ObjectAllocator &oa) = delete;
    ObjectAllocator &operator=(const ObjectAllocator &oa) = delete;

    //make private move ctor and move assignment
    ObjectAllocator(ObjectAllocator &&oa) = delete;
    ObjectAllocator &operator=(ObjectAllocator &&oa) = delete;
    
    // Other private fields and methods...
    //head pointer of the free block linked list
    GenericObject* freeList_ = nullptr; //containing available heap memory
     //head pointer of the page block linked list
    GenericObject* pageList_ = nullptr; //containing used up heap memory


};


inline void PrintCounts2(const ObjectAllocator *nm)
{
  OAStats stats = nm->GetStats();
  std::cout << "Pages in use: " << stats.PagesInUse_;
  std::cout << ", Objects in use: " << stats.ObjectsInUse_;
  std::cout << ", Available objects: " << stats.FreeObjects_;
  std::cout << ", Allocs: " << stats.Allocations_;
  std::cout << ", Frees: " << stats.Deallocations_ << std::endl;
}

/*
customer deleter
*/
struct OADeleter {
  ObjectAllocator* oa = nullptr;
  template <typename T>
  void operator()(T* p) const noexcept {
    if (!p) return;
    //if p has its own dtor, and already constructed
    //if construct with placement new must destroy explicitly in deleter
    p->~T();      
    if (oa) oa->Free(p);
  }
};
/* 
wrapper for unique ptr
template <class T, class Deleter = std::default_delete<T>>
class unique_ptr;

imagine T as Student, new ( placement-arguments ) Type(constructor-args)
T* obj = new (mem_from_mempool) T();    -> Student* s = new (mem) Student();
compiler translate special overload of operator new, then call ctor "at that adress"
-void* operator new(std::size_t, void* place) noexcept;
- new (mem) Student(); -> operator new(sizeof(Student), mem);

e.g. 
1) void* mem_from_mempool = studentObjMgr.Allocate();
-memory comes from custom memory pool
-no ctor run yet

2) new (mem) Student();
- does not allocate memory
- calls default ctor of Student at address mem
*/
template <typename T>
std::unique_ptr<T, OADeleter> oa_make_unique(ObjectAllocator& oa) {
  void* mem_from_mempool = oa.Allocate();
  // placement new (must have T default ctor!!!!)
  T* obj = new (mem_from_mempool) T();                 
  return std::unique_ptr<T, OADeleter>(obj, OADeleter{ &oa });
}

template <typename T>
std::shared_ptr<T> oa_make_shared(ObjectAllocator& oa)
{
    void* mem_from_mempool = oa.Allocate();
    T* obj = new (mem_from_mempool) T(); // default ctor
    return std::shared_ptr<T>(obj, OADeleter{ &oa });
}

#endif

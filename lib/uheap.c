#include <inc/lib.h>

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

int FirstTimeFlag = 1;
void InitializeUHeap()
{
	if(FirstTimeFlag)
	{
		initialize_dyn_block_system();
		cprintf("DYNAMIC BLOCK SYSTEM IS INITIALIZED\n");
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//=================================
void initialize_dyn_block_system() //run tm0 2000
{

	LIST_INIT(&AllocMemBlocksList);
	LIST_INIT(&FreeMemBlocksList);
	MAX_MEM_BLOCK_CNT=(USER_HEAP_MAX-USER_HEAP_START)/PAGE_SIZE;
	MemBlockNodes= (struct MemBlock*)USER_DYN_BLKS_ARRAY;
	uint32 s=ROUNDUP((MAX_MEM_BLOCK_CNT*sizeof(struct MemBlock)),PAGE_SIZE);
	sys_allocate_chunk( USER_DYN_BLKS_ARRAY,s,PERM_USER|PERM_WRITEABLE);
	initialize_MemBlocksList(MAX_MEM_BLOCK_CNT);
	struct MemBlock * Block=LIST_FIRST(&AvailableMemBlocksList);
	LIST_REMOVE(&AvailableMemBlocksList,Block);
	insert_sorted_with_merge_freeList(Block);
	Block->sva=USER_HEAP_START;
	Block->size=(USER_HEAP_MAX-USER_HEAP_START);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================

int32 user_array[(USER_HEAP_MAX - USER_HEAP_START) / PAGE_SIZE];
struct MemBlock *block;
void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	//==============================================================
	    size =ROUNDUP(size,PAGE_SIZE);

	    block = alloc_block_FF(size);
	    if(block == NULL){
	    	return NULL;
	    }else{
	    	uint32 start_address=block->sva;
	        insert_sorted_allocList(block);
	        return (void *)start_address;
	    }
		sys_isUHeapPlacementStrategyFIRSTFIT();
			block = alloc_block_FF(size);
			if(block !=NULL){
               uint32 start_address=block->sva;
				insert_sorted_allocList(block);
			return (void*) start_address;
			}

}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
// free():
//	This function frees the allocation of the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	FROM main memory AND free pages from page file then switch back to the user again.
//
//	We can use sys_free_user_mem(uint32 virtual_address, uint32 size); which
//		switches to the kernel mode, calls free_user_mem() in
//		"kern/mem/chunk_operations.c", then switch back to the user mode here
//	the free_user_mem function is empty, make sure to implement it.
void free(void* virtual_address)
{
	//wael
	    uint32 address =(uint32)ROUNDDOWN(virtual_address,PAGE_SIZE);
		struct MemBlock *search_block =find_block(&AllocMemBlocksList, address);
		if(search_block == NULL)
		{
			return(void)NULL;
		}else {
			sys_free_user_mem(address, search_block->size);
			LIST_REMOVE(&AllocMemBlocksList,search_block);
			insert_sorted_with_merge_freeList(search_block);
		}
}



//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================

	//TODO: [PROJECT MS3] [SHARING - USER SIDE] smalloc()
	// Write your code here, remove the panic and write your code
	//panic("smalloc() is not implemented yet...!!");
	// Steps:
	//	1) Implement FIRST FIT strategy to search the heap for suitable space
	//		to the required allocation size (space should be on 4 KB BOUNDARY)
	uint32 ret = sys_isUHeapPlacementStrategyFIRSTFIT();
	if(ret == 1)
	{
		uint32* va =malloc(size);
		//	2) if no suitable space found, return NULL
		if(va == NULL)
		{
			return NULL;
		}
		//	 Else,
		//	3) Call sys_createSharedObject(...) to invoke the Kernel for allocation of shared variable
		//		sys_createSharedObject(): if succeed, it returns the ID of the created variable. Else, it returns -ve
		//	4) If the Kernel successfully creates the shared variable, return its virtual address
		//	   Else, return NULL
		else
		{
			int ID = sys_createSharedObject(sharedVarName, size, isWritable, va);
			if(ID >= 0)//succeed
			{
				return va;
			}
		}
	}
	return NULL;

	//This function should find the space of the required range
	// ******** ON 4KB BOUNDARY ******************* //

	//Use sys_isUHeapPlacementStrategyFIRSTFIT() to check the current strategy
}



//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//wael
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
		int get_share_object_size=sys_getSizeOfSharedObject(ownerEnvID,sharedVarName);
		struct MemBlock *block_FirstFit;
		uint32 round_get_share_object_size=ROUNDUP(get_share_object_size,PAGE_SIZE);
		if(round_get_share_object_size==E_SHARED_MEM_NOT_EXISTS)
		{
			return NULL;
		}
		sys_isUHeapPlacementStrategyFIRSTFIT();
		block_FirstFit=alloc_block_FF(round_get_share_object_size);
		if(block_FirstFit==NULL)
		{
			return NULL;
		}
		int get_share_object =sys_getSharedObject(ownerEnvID,sharedVarName,(void*)block_FirstFit->sva);

		if(get_share_object==E_SHARED_MEM_EXISTS)
		{
			if(get_share_object==E_NO_SHARE)
			{
				return NULL;
			}
		}
		else
		{
			return (void *)block_FirstFit->sva;
		}
		return NULL;
}



//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//


//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// [USER HEAP - USER SIDE] realloc
	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
}


//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	//TODO: [PROJECT MS3 - BONUS] [SHARING - USER SIDE] sfree()

	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
	/*struct Env* myenv = curenv;
	struct Share *shares;
	struct FrameInfo * ptr_frame_info;
	int physical_address = sys_virtual_to_physical(myenv->env_page_directory,virtual_address);
	ptr_frame_info = to_frame_info(physical_address) ;
	for(int i = 0; i< MAX_SHARES ;i++)
	{
		if(shares[i].framesStorage == ptr_frame_info)
		{
			shares = shares[i];
			break;
		}
	}
	uint32 address =(uint32)ROUNDDOWN(virtual_address,PAGE_SIZE);
	struct MemBlock *search_block =find_block(&AllocMemBlocksList, address);
	if(search_block == NULL)
	{
		return(void)NULL;
	}else {
		//sys_free_user_mem(address, search_block->size);
		//freeSharedObject(int32 sharedObjectID, address);
		sys_freeSharedObject(shares->ownerID, address);
		LIST_REMOVE(&AllocMemBlocksList,search_block);
		insert_sorted_with_merge_freeList(search_block);
	}*/
}




//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//
void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");
}

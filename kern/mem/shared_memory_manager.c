#include <inc/memlayout.h>
#include "shared_memory_manager.h"

#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>
#include <inc/environment_definitions.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/syscall.h>
#include "kheap.h"
#include "memory_manager.h"

//2017

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//===========================
// [1] Create "shares" array:
//===========================
//Dynamically allocate the array of shared objects
//initialize the array of shared objects by 0's and empty = 1
void create_shares_array(uint32 numOfElements)
{
#if USE_KHEAP
	MAX_SHARES  = numOfElements ;
	shares = kmalloc(numOfElements*sizeof(struct Share));
	if (shares == NULL)
	{
		panic("Kernel runs out of memory\nCan't create the array of shared objects.");
	}
#endif
	for (int i = 0; i < MAX_SHARES; ++i)
	{
		memset(&(shares[i]), 0, sizeof(struct Share));
		shares[i].empty = 1;
	}
}

//===========================
// [2] Allocate Share Object:
//===========================
//Allocates a new (empty) shared object from the "shares" array
//It dynamically creates the "framesStorage"
//Return:
//	a) if succeed:
//		1. allocatedObject (pointer to struct Share) passed by reference
//		2. sharedObjectID (its index in the array) as a return parameter
//	b) E_NO_SHARE if the the array of shares is full (i.e. reaches "MAX_SHARES")
int allocate_share_object(struct Share **allocatedObject)
{
	int32 sharedObjectID = -1 ;
	for (int i = 0; i < MAX_SHARES; ++i)
	{
		if (shares[i].empty)
		{
			sharedObjectID = i;
			break;
		}
	}

	if (sharedObjectID == -1)
	{
		return E_NO_SHARE ;
/*		//try to increase double the size of the "shares" array
#if USE_KHEAP
		{
			shares = krealloc(shares, 2*MAX_SHARES);
			if (shares == NULL)
			{
				*allocatedObject = NULL;
				return E_NO_SHARE;
			}
			else
			{
				sharedObjectID = MAX_SHARES;
				MAX_SHARES *= 2;
			}
		}
#else
		{
			panic("Attempt to dynamically allocate space inside kernel while kheap is disabled .. ");
			*allocatedObject = NULL;
			return E_NO_SHARE;
		}
#endif
*/
	}

	*allocatedObject = &(shares[sharedObjectID]);
	shares[sharedObjectID].empty = 0;

#if USE_KHEAP
	{
		shares[sharedObjectID].framesStorage = create_frames_storage();
	}
#endif
	memset(shares[sharedObjectID].framesStorage, 0, PAGE_SIZE);

	return sharedObjectID;
}

//=========================
// [3] Get Share Object ID:
//=========================
//Search for the given shared object in the "shares" array
//Return:
//	a) if found: SharedObjectID (index of the shared object in the array)
//	b) else: E_SHARED_MEM_NOT_EXISTS
int get_share_object_ID(int32 ownerID, char* name)
{
	int i=0;

	for(; i< MAX_SHARES; ++i)
	{
		if (shares[i].empty)
			continue;

		//cprintf("shared var name = %s compared with %s\n", name, shares[i].name);
		if(shares[i].ownerID == ownerID && strcmp(name, shares[i].name)==0)
		{
			//cprintf("%s found\n", name);
			return i;
		}
	}
	return E_SHARED_MEM_NOT_EXISTS;
}

//=========================
// [4] Delete Share Object:
//=========================
//delete the given sharedObjectID from the "shares" array
//Return:
//	a) 0 if succeed
//	b) E_SHARED_MEM_NOT_EXISTS if the shared object is not exists
int free_share_object(uint32 sharedObjectID)
{
	if (sharedObjectID >= MAX_SHARES)
		return E_SHARED_MEM_NOT_EXISTS;

	//panic("deleteSharedObject: not implemented yet");
	clear_frames_storage(shares[sharedObjectID].framesStorage);
#if USE_KHEAP
	kfree(shares[sharedObjectID].framesStorage);
#endif
	memset(&(shares[sharedObjectID]), 0, sizeof(struct Share));
	shares[sharedObjectID].empty = 1;

	return 0;
}

// 2014 - edited in 2017
//===========================
// [5] Create frames_storage:
//===========================
// if KHEAP = 1: Create the frames_storage by allocating a PAGE for its directory
inline uint32* create_frames_storage()
{
	uint32* frames_storage = kmalloc(PAGE_SIZE);
	if(frames_storage == NULL)
	{
		panic("NOT ENOUGH KERNEL HEAP SPACE");
	}
	return frames_storage;
}
//===========================
// [6] Add frame to storage:
//===========================
// Add a frame info to the storage of frames at the given index
inline void add_frame_to_storage(uint32* frames_storage, struct FrameInfo* ptr_frame_info, uint32 index)
{
	uint32 va = index * PAGE_SIZE ;
	uint32 *ptr_page_table;
	int r = get_page_table(frames_storage,  va, &ptr_page_table);
	if(r == TABLE_NOT_EXIST)
	{
#if USE_KHEAP
		{
			ptr_page_table = create_page_table(frames_storage, (uint32)va);
		}
#else
		{
			__static_cpt(frames_storage, (uint32)va, &ptr_page_table);

		}
#endif
	}
	ptr_page_table[PTX(va)] = CONSTRUCT_ENTRY(to_physical_address(ptr_frame_info), 0 | PERM_PRESENT);
}

//===========================
// [7] Get frame from storage:
//===========================
// Get a frame info from the storage of frames at the given index
inline struct FrameInfo* get_frame_from_storage(uint32* frames_storage, uint32 index)
{
	struct FrameInfo* ptr_frame_info;
	uint32 *ptr_page_table ;
	uint32 va = index * PAGE_SIZE ;
	ptr_frame_info = get_frame_info(frames_storage,  va, &ptr_page_table);
	return ptr_frame_info;
}

//===========================
// [8] Clear the frames_storage:
//===========================
inline void clear_frames_storage(uint32* frames_storage)
{
	int fourMega = 1024 * PAGE_SIZE ;
	int i ;
	for (i = 0 ; i < 1024 ; i++)
	{
		if (frames_storage[i] != 0)
		{
#if USE_KHEAP
			{
				kfree((void*)kheap_virtual_address(EXTRACT_ADDRESS(frames_storage[i])));
			}
#else
			{
				free_frame(to_frame_info(EXTRACT_ADDRESS(frames_storage[i])));
			}
#endif
			frames_storage[i] = 0;
		}
	}
}


//==============================
// [9] Get Size of Share Object:
//==============================
int getSizeOfSharedObject(int32 ownerID, char* shareName)
{
	// your code is here, remove the panic and write your code
	//panic("getSizeOfSharedObject() is not implemented yet...!!");

	// This function should return the size of the given shared object
	// RETURN:
	//	a) If found, return size of shared object
	//	b) Else, return E_SHARED_MEM_NOT_EXISTS
	//

	int shareObjectID = get_share_object_ID(ownerID, shareName);
	if (shareObjectID == E_SHARED_MEM_NOT_EXISTS)
		return E_SHARED_MEM_NOT_EXISTS;
	else
		return shares[shareObjectID].size;

	return 0;
}

//********************************************************************************//

//===========================================================


//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=========================
// [1] Create Share Object:
//=========================
int createSharedObject(int32 ownerID, char* shareName, uint32 size, uint8 isWritable, void* virtual_address)
	{
	//mariam new
		//TODO: [PROJECT MS3] [SHARING - KERNEL SIDE] createSharedObject()
		//panic("createSharedObject() is not implemented yet...!!");
		//	a) ShareObjectID (its index in "shares" array) if success
		//	b) E_SHARED_MEM_EXISTS if the shared object already exists
		//	c) E_NO_SHARE if the number of shared objects reaches max "MAX_SHARES"
		struct Env* myenv = curenv; //The calling environment
		int zeft;
		int gettingID = get_share_object_ID(ownerID , shareName);
		int Share_object_ID ;
		if(gettingID == E_SHARED_MEM_NOT_EXISTS)
		{
		//1st: Allocate a new share object (use allocate_share_object())
		struct Share *shared_obj;
		Share_object_ID = allocate_share_object(&shared_obj);
		//2nd: Allocate the required space in the physical memory on a PAGE boundary @allocate_frame
		if ((Share_object_ID != E_NO_SHARE) && (virtual_address != NULL))
		{
			int sizee =ROUNDUP((uint32)virtual_address+size,PAGE_SIZE);
			uint32 start= ROUNDDOWN((uint32)virtual_address,PAGE_SIZE);
			int i = start;
			uint32 *ptr_page_table=NULL;
			for(int i=start; i<sizee; i+=PAGE_SIZE){
				uint32 va_ADD= (uint32)virtual_address;
			struct FrameInfo *frameInfo = get_frame_info(myenv->env_page_directory,i, &ptr_page_table);
			va_ADD+=PAGE_SIZE;

			if ( frameInfo!= NULL){
					return -1;
				}
			}
			while(i<sizee)
			{
				struct FrameInfo *frameInfo = get_frame_info(myenv->env_page_directory,i, &ptr_page_table);


				allocate_frame(&frameInfo);
					//3rd: Map the allocated space on the given "virtual_address" on the current environment "myenv": object OWNER,
					map_frame(myenv->env_page_directory, frameInfo,i, PERM_USER|PERM_WRITEABLE);
					add_frame_to_storage(shared_obj->framesStorage, frameInfo, Share_object_ID);
					frameInfo->va=i;
				//}

				i+=PAGE_SIZE;
			}
			//4th: Initialization
			shared_obj->ownerID = ownerID;
			shared_obj->isWritable = isWritable;
			shared_obj->references = 1;
			shared_obj->size = size;
			strcpy(shared_obj->name, shareName);
		}

		else if(Share_object_ID == E_NO_SHARE)
		{
			return E_NO_SHARE;
		}
		}
	else if(gettingID != E_SHARED_MEM_NOT_EXISTS)
	{
		return E_SHARED_MEM_EXISTS;
	}
		return Share_object_ID;
}


//======================
// [2] Get Share Object:
//======================
int getSharedObject(int32 ownerID, char* shareName, void* virtual_address){
	//wael
	uint32 obj = get_share_object_ID(ownerID,shareName);
	struct FrameInfo*  info = get_frame_from_storage(shares[obj].framesStorage,obj);
	info->va=(uint32) virtual_address;
	switch(shares[obj].isWritable){
	int ret;
	case 1 :
		ret = map_frame(curenv->env_page_directory,info,(uint32)virtual_address,PERM_USER|PERM_WRITEABLE|PERM_PRESENT);
		if(ret != 0){
			return E_SHARED_MEM_NOT_EXISTS;
		}else{
			shares[obj].references++;
			return shares[obj].ownerID;
		}
	case 0 :
		ret= map_frame(curenv->env_page_directory,info,(uint32)virtual_address,PERM_USER|PERM_PRESENT);
		if(ret != 0){
			return E_SHARED_MEM_NOT_EXISTS;
		}else{
			shares[obj].references++;
			return shares[obj].ownerID;
		}
	}
	return E_SHARED_MEM_NOT_EXISTS;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//===================
// Free Share Object:
//===================
int freeSharedObject(int32 sharedObjectID, void *startVA)
{
	//TODO: [PROJECT MS3 - BONUS] [SHARING - KERNEL SIDE] freeSharedObject()
	// your code is here, remove the panic and write your code
	//panic("freeSharedObject() is not implemented yet...!!");

	struct Env* myenv = curenv; //The calling environment

	// This function should free (delete) the shared object from the User Heapof the current environment
	// If this is the last shared env, then the "frames_store" should be cleared and the shared object should be deleted
	// RETURN:
	//	a) 0 if success
	//	b) E_SHARED_MEM_NOT_EXISTS if the shared object is not exists

	// Steps:
	//	1) Get the shared object from the "shares" array (use get_share_object_ID())
	//	2) Unmap it from the current environment "myenv"
	//	3) If one or more table becomes empty, remove it
	//	4) Update references
	//	5) If this is the last share, delete the share object (use free_share_object())
	//	6) Flush the cache "tlbflush()"
	////////////////////////////////////////////////////////////////////////////////////////////

	uint32 address=ROUNDDOWN((uint32)startVA,PAGE_SIZE);

	//	1) Get the shared object from the "shares" array (use get_share_object_ID())
	int32 shareid=get_share_object_ID(shares[sharedObjectID].ownerID, shares[sharedObjectID].name);
	if(shareid==E_SHARED_MEM_NOT_EXISTS)
		return E_SHARED_MEM_NOT_EXISTS;

	//	2) Unmap it from the current environment "myenv"
	unmap_frame((uint32*)myenv->env_page_directory,address);

	//	3) If one or more table becomes empty, remove it
	int empty; //flag
	int SIZE=shares[sharedObjectID].size;
	uint32 *ptr_page_table = NULL;

	for(uint32 i=address;i<address+SIZE;i+=PAGE_SIZE)//each page table
	{
		empty = 0;///////1
		///////2
		struct FrameInfo *ptr_frame_info = get_frame_info(myenv->env_page_directory,i,&ptr_page_table);
		//get_page_table(e->env_page_directory, i, &ptr_page_table);
		if(ptr_page_table != NULL)///////3
		{
			for(uint32 j = 0;j<1024;j++)//each page in page table
			{

				if(ptr_page_table[j] != 0) //////4
					//means it is mapped
				{
					empty = 1; //not empty
					break;
				}
			}
			if(empty == 0) //page table is empty
			{
				kfree(ptr_page_table);
				//uint32 directory_entry = e->env_page_directory[PDX(i)];
				myenv->env_page_directory[PDX(i)] = 0; //pd_clear_page_dir_entry
			}
		}
	}

	//	4) Update references
	shares[shareid].references--;

	//	5) If this is the last share, delete the share object (use free_share_object())
	if(shares[shareid].references==0){
		free_share_object(shareid);
	}
	//shares

	//	6) Flush the cache "tlbflush()"
	tlbflush();
	return 0;
}


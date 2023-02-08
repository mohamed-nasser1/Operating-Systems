/*
 * paging_helpers.c
 *
 *  Created on: Sep 30, 2022
 *      Author: HP
 */
#include "memory_manager.h"

/*[2.1] PAGE TABLE ENTRIES MANIPULATION */
inline void pt_set_page_permissions(uint32* page_directory, uint32 virtual_address, uint32 permissions_to_set, uint32 permissions_to_clear)
{
	uint32 directory_entry = page_directory[PDX(virtual_address)];

	uint32 *ptr_page_table = NULL;
	get_page_table(page_directory, virtual_address, &ptr_page_table);

	if (ptr_page_table  != NULL)
	{
	  uint32 table_entry = ptr_page_table [PTX(virtual_address)];

	  if(permissions_to_set){
//		  uint32 new =permissions_to_set | 1;
		  ptr_page_table[PTX(virtual_address)] = ptr_page_table[PTX(virtual_address)] | (permissions_to_set);

	  }
	  if(permissions_to_clear){
		  ptr_page_table[PTX(virtual_address)] = ptr_page_table[PTX(virtual_address)] & (~permissions_to_clear);

	  }


//	ptr_page_table[PTX(permissions_to_set)] = ptr_page_table[PTX(virtual_address)] ^ (1);
//	ptr_page_table[PTX(permissions_to_clear)] = ptr_page_table[PTX(virtual_address)] & (0);
	tlb_invalidate((void*)NULL,(void*)virtual_address);
	}
	else{
		panic("pt_set not exit & no page table...!!");
	}
}


inline int pt_get_page_permissions(uint32* page_directory, uint32 virtual_address )
{
		uint32 directory_entry = page_directory[PDX(virtual_address)];
		int frameNum = directory_entry;

		uint32 *ptr_page_table = NULL;
		int ret = get_page_table(page_directory, virtual_address, &ptr_page_table);
		if (ret == TABLE_IN_MEMORY) {
			uint32 table_entry = ptr_page_table [PTX(virtual_address)];
			frameNum = table_entry ;
			uint32 x=frameNum & 0x00000fff;
			return(x);
		}
		return(-1);

	//TODO: [PROJECT MS2] [PAGING HELPERS] pt_get_page_permissions
	// Write your code here, remove the panic and write your code
	/*panic("pt_get_page_permissions() is not implemented yet...!!");*/

}

inline void pt_clear_page_table_entry(uint32* page_directory, uint32 virtual_address)
{
	//TODO: [PROJECT MS2] [PAGING HELPERS] pt_clear_page_table_entry
	// Write your code here, remove the panic and write your code
	uint32 *ptr_page_table = NULL;
	int ret = get_page_table(page_directory, virtual_address, &ptr_page_table);
	if(ret == TABLE_IN_MEMORY)  // or if(ptr_page_table != NULL)
	{
		unmap_frame(page_directory, virtual_address);
	}
	else
	{
		panic("Invalid va");
	}
	tlb_invalidate((void *)NULL, (void *)virtual_address);
	//panic("pt_clear_page_table_entry() is not implemented yet...!!");
}

/***********************************************************************************************/

/*[2.2] ADDRESS CONVERTION*/
inline int virtual_to_physical(uint32* page_directory, uint32 virtual_address)
{
	//TODO: [PROJECT MS2] [PAGING HELPERS] virtual_to_physical
	// Write your code here, remove the panic and write your code
	/*uint32 directory_entry = ptr_page_directory[PDX(virtual_address)];
	int frame_num = directory_entry>>12;*/
	int frame_num;
	uint32 *ptr_page_table = NULL;
	int ret = get_page_table(page_directory, virtual_address, &ptr_page_table);
	if(ret == TABLE_IN_MEMORY)  // or if(ptr_page_table != NULL)
	{
		uint32 table_entry = ptr_page_table[PTX(virtual_address)];
		frame_num = table_entry>>12;
		int pa = frame_num * PAGE_SIZE;
		return pa;
	}
	else
	{
		return -1;
	}
	//panic("virtual_to_physical() is not implemented yet...!!");
}
/***********************************************************************************************/

/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/
/***********************************************************************************************/

///============================================================================================
/// Dealing with page directory entry flags

inline uint32 pd_is_table_used(uint32* page_directory, uint32 virtual_address)
{
	return ( (page_directory[PDX(virtual_address)] & PERM_USED) == PERM_USED ? 1 : 0);
}

inline void pd_set_table_unused(uint32* page_directory, uint32 virtual_address)
{
	page_directory[PDX(virtual_address)] &= (~PERM_USED);
	tlb_invalidate((void *)NULL, (void *)virtual_address);
}

inline void pd_clear_page_dir_entry(uint32* page_directory, uint32 virtual_address)
{
	page_directory[PDX(virtual_address)] = 0 ;
	tlbflush();
}

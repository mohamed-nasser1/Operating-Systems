/*
 * chunk_operations.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include <kern/trap/fault_handler.h>
#include <kern/disk/pagefile_manager.h>
#include "kheap.h"
#include "memory_manager.h"



/******************************/
/*[1] RAM CHUNKS MANIPULATION */
/******************************/

//===============================
// 1) CUT-PASTE PAGES IN RAM:
//===============================

int cut_paste_pages(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 num_of_pages)
{
	int perm[num_of_pages];
	struct FrameInfo *point[num_of_pages];
	int count=0;
	int index =PDX(dest_va);
	//check dest
	uint32 directory_entry = page_directory[PDX(dest_va)];
	uint32 *ptr_page_table = NULL;
	for(int i=dest_va;i<dest_va+(PAGE_SIZE*num_of_pages);i+=PAGE_SIZE){
		struct FrameInfo *ptr_frame_info = get_frame_info(page_directory,i,&ptr_page_table);
		if(ptr_frame_info !=NULL){
		    return -1;
		}
	 }
	for(int i=dest_va;i<dest_va+(PAGE_SIZE*num_of_pages);i+=PAGE_SIZE){
			get_page_table(page_directory, i, &ptr_page_table);
			if(ptr_page_table == NULL){
					ptr_page_table = create_page_table(page_directory, i);
				}
		}
	//get perm
	uint32 directory_entry_S = page_directory[PDX(source_va)];
	int frameNum;
	uint32 *ptr_page_tables = NULL;
	int rets = get_page_table(page_directory, source_va, &ptr_page_tables);
	if (rets == TABLE_IN_MEMORY) {
		for(int i = source_va ; i<source_va+(PAGE_SIZE*num_of_pages);i+=PAGE_SIZE){
			struct FrameInfo *ptr_frame_info=get_frame_info(page_directory,i,&ptr_page_tables);
			point[count]=ptr_frame_info;
			uint32 table_entry = ptr_page_tables[PTX(i)];
			frameNum = table_entry ;
			uint32 x=frameNum & 0x00000fff;
			perm[count]=x;
			count++;
		}
	}

	//cut
	count=0;
	for(int i = dest_va;i<(dest_va+(PAGE_SIZE*num_of_pages));i+=PAGE_SIZE){
		struct FrameInfo *ptr_frame_info=point[count];
		map_frame(page_directory,ptr_frame_info,i,perm[count]);
		count++;
		}
	//delete pointer
	for(int i = source_va ; i<source_va+(PAGE_SIZE*num_of_pages);i+=PAGE_SIZE){
	    unmap_frame(page_directory,i);
	}

return 0;

}
//===============================
// 2) COPY-PASTE RANGE IN RAM:
//===============================
uint32 *ptr_page_table_dest = NULL;
int copy_paste_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 size)
{
    //struct FrameInfo *point[num_of_pages];
	int count=0;
	int perm[count];
	//check writable && exist
	for(int i=dest_va;i<dest_va+size;i+=PAGE_SIZE){
		uint32 get_perm = pt_get_page_permissions(page_directory,i) & PERM_WRITEABLE;
		struct FrameInfo *ptr_frame_info = get_frame_info(page_directory,i,&ptr_page_table_dest);
		if(ptr_frame_info !=NULL && get_perm == 0)
		{
			return -1;
		}
	}
	uint32 j_dest=dest_va;
	uint32 j_source=source_va;
	for(int i=dest_va;i<dest_va+size;i+=PAGE_SIZE){
		get_page_table(page_directory,j_dest,&ptr_page_table_dest);
		if(ptr_page_table_dest ==NULL){
			create_page_table(page_directory,j_dest);
		}
		uint32 count_dest=j_dest;
		uint32 count_source=j_source;
		struct FrameInfo *ptr_frame_info = get_frame_info(page_directory,j_dest,&ptr_page_table_dest);
		uint32 get_perm = pt_get_page_permissions(page_directory,j_dest) & PERM_WRITEABLE;
		if(ptr_frame_info !=NULL && get_perm != 0){
				for(int j =0 ;j<PAGE_SIZE&&count_dest<=dest_va+size;j++){
					unsigned char *source = (unsigned char *) (count_source);
					unsigned char *dest = (unsigned char *) (count_dest);
					*dest=*source;
					count_dest++;
					count_source++;
				}
		}
		else if(ptr_frame_info ==NULL){
			allocate_frame(&ptr_frame_info);
			int perm = pt_get_page_permissions(page_directory,j_source);
			map_frame(page_directory, ptr_frame_info, j_dest,perm);
				for(int j =0 ;j<PAGE_SIZE&&count_dest<=dest_va+size;j++){
					unsigned char *source = (unsigned char *) (count_source);
					unsigned char *dest = (unsigned char *) (count_dest);
					*dest=*source;
					count_dest++;
					count_source++;
				}
		}
		else {
			return -1;
		}
		j_dest+=PAGE_SIZE;
		j_source+=PAGE_SIZE;
	}

return 0;

}

//===============================
// 3) SHARE RANGE IN RAM:
//===============================
int share_chunk(uint32* page_directory, uint32 source_va,uint32 dest_va, uint32 size, uint32 perms)
{
	uint32 s_va = ROUNDDOWN(source_va,PAGE_SIZE);
	uint32 d_va = ROUNDDOWN(dest_va,PAGE_SIZE);
	uint32 ed_va = ROUNDUP(dest_va + size,PAGE_SIZE);
	uint32 es_va = ROUNDUP(source_va + size,PAGE_SIZE);
	uint32 no_of_pages=(es_va-s_va)/PAGE_SIZE;

	int perm[no_of_pages];
	struct FrameInfo *point[no_of_pages];
	int count=0;

	//check dest
	uint32 directory_entry = page_directory[PDX(d_va)];
	uint32 *ptr_page_table = NULL;


	for(int i=d_va;i<d_va+(PAGE_SIZE*no_of_pages);i+=PAGE_SIZE){
		struct FrameInfo *ptr_frame_info = get_frame_info(page_directory,i,&ptr_page_table);
		if(ptr_frame_info !=NULL){
			return -1;
		}
	 }
	for(int i=d_va;i<d_va+(PAGE_SIZE*no_of_pages);i+=PAGE_SIZE){
			get_page_table(page_directory, i, &ptr_page_table);
			if(ptr_page_table == NULL){
					ptr_page_table = create_page_table(page_directory, i);
				}
		}

	uint32 *ptr_page_tables = NULL;

	count=0;
	for(int i = s_va ; i<s_va+(PAGE_SIZE*no_of_pages);i+=PAGE_SIZE){
		struct FrameInfo *ptr_frame_info=get_frame_info(page_directory,i,&ptr_page_tables);
		point[count]=ptr_frame_info;
		count++;
		}


	//cut
	count=0;
	for(int i = d_va;i<(d_va+(PAGE_SIZE*no_of_pages));i+=PAGE_SIZE){
			struct FrameInfo *ptr_frame_info=point[count];
			map_frame(page_directory,ptr_frame_info,i,perms);
			count++;
		}
return 0;
}

//===============================
// 4) ALLOCATE CHUNK IN RAM:
//===============================
int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms)
{

	uint32 *ptr_page_table = NULL;
	int va_f = ROUNDDOWN(va,PAGE_SIZE);
	int va_l = ROUNDUP(va+size,PAGE_SIZE);
	int new_sva=va_f;

	//check exit
	for(int i=va_f; i<va_l ;i+=PAGE_SIZE){
		ptr_page_table = NULL;
		struct FrameInfo *ptr_frame_info = get_frame_info(page_directory,i,&ptr_page_table);
		if(ptr_frame_info !=NULL){
			return -1;
		}
	 }
	//creat table
	for(int i=va_f;i<va_l;i+=PAGE_SIZE){
		get_page_table(page_directory, i, &ptr_page_table);
		if(ptr_page_table == NULL){
				ptr_page_table = create_page_table(page_directory, i);
			}
	}
	//allocate & map
	for(int i=va_f; i<va_l ;i+=PAGE_SIZE){
		ptr_page_table = NULL;
		struct FrameInfo *ptr_frame_info = get_frame_info(page_directory,i,&ptr_page_table);
		int ret1 = allocate_frame(&ptr_frame_info);
		if(ret1==E_NO_MEM)
			{new_sva=i; break;}
		int ret2 = map_frame(page_directory,ptr_frame_info,i,perms);
		if(ret2==E_NO_MEM)
			{new_sva=i; break;}
		ptr_frame_info->va=i;
	}

	//free
	for(int i=va_f; i<new_sva && new_sva!=va_l ;i+=PAGE_SIZE){
		ptr_page_table = NULL;
		struct FrameInfo *ptr_frame_info = get_frame_info(page_directory,i,&ptr_page_table);
		free_frame(ptr_frame_info);
	}

		return 0;
}
int findDuplicates(int arr[], int n)
{
  // create another array of similar size
  int temp[n];
  int count = 0;
    int num;
  // traverse original array
  // (don't check first element)

  for(int i=0;i<n;i++){
      num=arr[i];
      int b=1;
      for(int j=0;j<count;j++){
          if(temp[j]==num)
                b=0;
      }
      if(b==1)
      temp[count++]=num;

  }

 return count;

}
/*BONUS*/
//=====================================
// 5) CALCULATE ALLOCATED SPACE IN RAM:
//=====================================
void calculate_allocated_space(uint32* page_directory, uint32 sva, uint32 eva, uint32 *num_tables, uint32 *num_pages)
{

	uint32 va_f = ROUNDDOWN(sva,PAGE_SIZE);
	uint32 va_l = ROUNDUP(eva,PAGE_SIZE);
	uint32 *ptr_page_table = NULL;

    ///num of page
	uint32 cpage=0;
	for(int i = va_f ; i<=va_l;i+=PAGE_SIZE){
		struct FrameInfo *ptr_frame_info = get_frame_info(page_directory,i,&ptr_page_table);
		if(ptr_page_table!=NULL)
			if((ptr_page_table[PTX(i)] &PERM_PRESENT) !=0)
				cpage++;
	}

	///num of table
	int temp[100];
	uint32 ctable=0;
		for(int i = va_f ; i<=va_l;i+=PAGE_SIZE){
		int ret =get_page_table(page_directory, i, &ptr_page_table);
		if(ret==TABLE_IN_MEMORY ){
			int test=1;
			for(int i=0;i<ctable;i++){
				if(*ptr_page_table==temp[i])test=0;

			}
			if(test==1)
				{temp[ctable++]=*ptr_page_table;}
		}
	}

	//final
	*(uint32*)num_pages=cpage;
	*(uint32*)num_tables=ctable;

}

/*BONUS*/
//=====================================
// 6) CALCULATE REQUIRED FRAMES IN RAM:
//=====================================

uint32 calculate_required_frames(uint32* page_directory, uint32 sva, uint32 size)
{

	int counter=0;
	uint32 va=ROUNDDOWN(sva,PAGE_SIZE);
	uint32 va_pg_idx=ROUNDDOWN(va,PAGE_SIZE*1024);
	int table_idx=PAGE_SIZE*1024;
	uint32* ptr_page;
	struct FrameInfo *ptr_frame_info=NULL;
	uint32 index=va;

	//free frame
	while(index<sva+size){
		int re=get_page_table(page_directory,index,&ptr_page);
		ptr_frame_info=get_frame_info(page_directory,index,&ptr_page);
		if(ptr_frame_info==NULL){
			counter++;
		}
		index+=PAGE_SIZE;
	}
		//table not exit
	while(va_pg_idx<sva+size){
			int re=get_page_table(page_directory,va_pg_idx,&ptr_page);
			if(ptr_page==NULL){
				counter++;
			}
			va_pg_idx+=table_idx;
		}


	return counter;
}



//=================================================================================//
//===========================END RAM CHUNKS MANIPULATION ==========================//
//=================================================================================//

/*******************************/
/*[2] USER CHUNKS MANIPULATION */
/*******************************/

//======================================================
/// functions used for USER HEAP (malloc, free, ...)
//======================================================

//=====================================
// 1) ALLOCATE USER MEMORY:
//=====================================
void allocate_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	// Write your code here, remove the panic and write your code
	panic("allocate_user_mem() is not implemented yet...!!");
}

//=====================================
// 2) FREE USER MEMORY:
//=====================================
void free_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	//TODO: [PROJECT MS3] [USER HEAP - KERNEL SIDE] free_user_mem
	// Write your code here, remove the panic and write your code
	//panic("free_user_mem() is not implemented yet...!!");
	uint32 s_va = ROUNDDOWN(virtual_address,PAGE_SIZE);
	uint32 end_va = s_va + size;
	uint32 no_of_pages=(end_va-s_va)/PAGE_SIZE;

	//1.Free ALL pages of the given range from the Page File
	//2.Free ONLY pages that are resident in the working set from the memory
	for(uint32 i = s_va ; i<end_va;i+=PAGE_SIZE)
	{
		unmap_frame(e->env_page_directory, i);
		pf_remove_env_page(e, i);
		env_page_ws_invalidate(e, i);
	}
	//3.Removes ONLY the empty page tables
	int empty; //flag

	uint32 *ptr_page_table = NULL;
	for(uint32 i=s_va;i<end_va;i+=PAGE_SIZE)//each page table
	{
		empty = 0;///////1
		///////2
		struct FrameInfo *ptr_frame_info = get_frame_info(e->env_page_directory,i,&ptr_page_table);
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
				e->env_page_directory[PDX(i)] = 0; //pd_clear_page_dir_entry
			}
		}
	}
}
//=====================================
// 2) FREE USER MEMORY (BUFFERING):
//=====================================
void __free_user_mem_with_buffering(struct Env* e, uint32 virtual_address, uint32 size)
{
	// your code is here, remove the panic and write your code
	panic("__free_user_mem_with_buffering() is not implemented yet...!!");

	//This function should:
	//1. Free ALL pages of the given range from the Page File
	//2. Free ONLY pages that are resident in the working set from the memory
	//3. Free any BUFFERED pages in the given range
	//4. Removes ONLY the empty page tables (i.e. not used) (no pages are mapped in the table)
}

//=====================================
// 3) MOVE USER MEMORY:
//=====================================
void move_user_mem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
{
	//TODO: [PROJECT MS3 - BONUS] [USER HEAP - KERNEL SIDE] move_user_mem
	//your code is here, remove the panic and write your code
	panic("move_user_mem() is not implemented yet...!!");

	// This function should move all pages from "src_virtual_address" to "dst_virtual_address"
	// with the given size
	// After finished, the src_virtual_address must no longer be accessed/exist in either page file
	// or main memory

	/**/
}

//=================================================================================//
//========================== END USER CHUNKS MANIPULATION =========================//
//=================================================================================//


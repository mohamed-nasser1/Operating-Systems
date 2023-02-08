#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"







void initialize_dyn_block_system()
{
	  LIST_INIT(&AllocMemBlocksList);
	  LIST_INIT(&FreeMemBlocksList);
	  MAX_MEM_BLOCK_CNT=(KERNEL_HEAP_MAX-KERNEL_HEAP_START)/PAGE_SIZE;
	  MemBlockNodes= (struct MemBlock*)KERNEL_HEAP_START;
	  uint32 s=ROUNDUP((MAX_MEM_BLOCK_CNT*sizeof(struct MemBlock)),PAGE_SIZE);
	  allocate_chunk(ptr_page_directory, KERNEL_HEAP_START,s,PERM_WRITEABLE);
	  initialize_MemBlocksList(MAX_MEM_BLOCK_CNT);
	  struct MemBlock * Block=LIST_FIRST(&AvailableMemBlocksList);
	  LIST_REMOVE(&AvailableMemBlocksList,Block);
	  insert_sorted_with_merge_freeList(Block);
	  Block->size=(KERNEL_HEAP_MAX-KERNEL_HEAP_START)-s;
	  Block->sva=KERNEL_HEAP_START+s;
}




struct MemBlock *block;
uint32 f= KERNEL_HEAP_START;

void* kmalloc(unsigned int size)
{
	size =ROUNDUP(size,PAGE_SIZE);

	if(KERNEL_HEAP_MAX- f> size ){
	if(isKHeapPlacementStrategyFIRSTFIT()){
		block = alloc_block_FF(size);
		if(block != NULL){
			insert_sorted_allocList(block);
			allocate_chunk(ptr_page_directory,block->sva,block->size,PERM_WRITEABLE);
			return(void *) block->sva;
		}else {
			return NULL;
		}
	}
	if(isKHeapPlacementStrategyBESTFIT()){
		block =alloc_block_BF(size);
		if(block == NULL){
			return NULL;
		}else {
			insert_sorted_allocList(block);
			allocate_chunk(ptr_page_directory,block->sva,block->size,PERM_WRITEABLE);
			return(void *) block->sva;
		}
	}
	if(isKHeapPlacementStrategyNEXTFIT()){
			block =alloc_block_NF(size);
			if(block == NULL){
				return NULL;
			}else {
				insert_sorted_allocList(block);
				allocate_chunk(ptr_page_directory,block->sva,block->size,PERM_WRITEABLE);
				return(void *) block->sva;
			}
		}
	}else {
	return NULL;
	}
	return NULL;
}




void kfree(void* virtual_address)
{
	struct MemBlock *prt_block = find_block(&AllocMemBlocksList, (uint32) virtual_address);
	if (prt_block!=NULL){

		uint32 sva = prt_block->sva;
			uint32 block_size = prt_block->size;
			uint32 end_va = sva + block_size;
			uint32 no_of_pages=(end_va-sva)/PAGE_SIZE;
	     LIST_REMOVE(&AllocMemBlocksList,prt_block);


	uint32 directory_entry = ptr_page_directory[PDX(sva)];
	for(int i = sva ; i<sva+(PAGE_SIZE*no_of_pages);i+=PAGE_SIZE){
	unmap_frame(ptr_page_directory,i);
	}


	insert_sorted_with_merge_freeList(prt_block);

	}
}




unsigned int kheap_virtual_address(unsigned int physical_address)
{
	struct FrameInfo *f = to_frame_info(physical_address);

	if(f != NULL){
	uint32 virtual_s = f->va;
	return (unsigned int) virtual_s;
	}else {
		return 0;
	}

}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	uint32 i=0 ;
    uint32 paaa ;
    uint32* ptr_table = NULL;
    if (i==0){
	get_page_table(ptr_page_directory, (uint32) virtual_address, &ptr_table);
	paaa = (ptr_table[PTX(virtual_address)] >> 12) * PAGE_SIZE;
    }
    return paaa;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}





void *krealloc(void *virtual_address, uint32 new_size)
{

	struct MemBlock *prt_block = find_block(&AllocMemBlocksList, (uint32) virtual_address);

	if(virtual_address == NULL && new_size != 0) //kmalloc only
	{
		void* returnn = kmalloc(new_size);
		return returnn;
	}
	else if(virtual_address != NULL && new_size == 0) //kfree only
	{
		kfree(virtual_address);
		return NULL;
	}
	else if(virtual_address != NULL && new_size == prt_block->size)
	{
		return virtual_address;
	}
	else
	{
		void* returnn = kmalloc(new_size);
		if(returnn == NULL)
		{
			return NULL;
		}
		else
		{
			kfree(virtual_address);
			return returnn;
		}

	}

}


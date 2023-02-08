/*
 * dyn_block_management.c
 *
 *  Created on: Sep 21, 2022
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//===========================
// PRINT MEM BLOCK LISTS:
//===========================

void print_mem_block_lists()
{
	cprintf("\n=========================================\n");
	struct MemBlock* blk ;
	struct MemBlock* lastBlk = NULL ;
	cprintf("\nFreeMemBlocksList:\n");
	uint8 sorted = 1 ;
	LIST_FOREACH(blk, &FreeMemBlocksList)
	{
		if (lastBlk && blk->sva < lastBlk->sva + lastBlk->size)
			sorted = 0 ;
		cprintf("[%x, %x)-->", blk->sva, blk->sva + blk->size) ;
		lastBlk = blk;
	}
	if (!sorted)	cprintf("\nFreeMemBlocksList is NOT SORTED!!\n") ;

	lastBlk = NULL ;
	cprintf("\nAllocMemBlocksList:\n");
	sorted = 1 ;
	LIST_FOREACH(blk, &AllocMemBlocksList)
	{
		if (lastBlk && blk->sva < lastBlk->sva + lastBlk->size)
			sorted = 0 ;
		cprintf("[%x, %x)-->", blk->sva, blk->sva + blk->size) ;
		lastBlk = blk;
	}
	if (!sorted)	cprintf("\nAllocMemBlocksList is NOT SORTED!!\n") ;
	cprintf("\n=========================================\n");

}

//********************************************************************************//
//********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//===============================
// [1] INITIALIZE AVAILABLE LIST:
//===============================
void initialize_MemBlocksList(uint32 numOfBlocks)
{
	LIST_INIT(&AvailableMemBlocksList);
	LIST_INIT(&AllocMemBlocksList);
	LIST_INIT(&FreeMemBlocksList);
	for(int i=0 ; i<numOfBlocks ; i++) {
           if(i==0){
			LIST_INSERT_HEAD(&AvailableMemBlocksList, &(MemBlockNodes[i]));
           }else {
        	   LIST_INSERT_TAIL(&AvailableMemBlocksList, &(MemBlockNodes[i]));
           }
			MemBlockNodes[i].size =0 ;
			MemBlockNodes[i].sva=0 ;
			AvailableMemBlocksList.size == numOfBlocks ;
	}
}


//===============================
// [2] FIND BLOCK:
//===============================
struct MemBlock *find_block(struct MemBlock_List *blockList, uint32 va)
{
	struct MemBlock *ptr ;
	struct MemBlock * point = NULL;
	LIST_FOREACH( ptr ,blockList){
		if (ptr->sva == va)
		{
			point= ptr ;
		}

	}
	return point ;
}


//=========================================
// [3] INSERT BLOCK IN ALLOC LIST [SORTED]:
//=========================================
void insert_sorted_allocList(struct MemBlock *blockToInsert)
{
	struct MemBlock *firstblock=LIST_FIRST(&AllocMemBlocksList) ;
	struct MemBlock *lastblock=LIST_LAST(&AllocMemBlocksList);

	if(AllocMemBlocksList.size == 0){
		LIST_INSERT_HEAD(&AllocMemBlocksList , blockToInsert);
		}
	else if(blockToInsert->sva >= lastblock->sva){
		LIST_INSERT_TAIL(&AllocMemBlocksList, blockToInsert);
	}else if(firstblock->sva > blockToInsert->sva){
		LIST_INSERT_HEAD(&AllocMemBlocksList, blockToInsert);
	}
	else
	{
		uint32 sva = blockToInsert->sva;
		while(firstblock->sva < sva){
			firstblock = firstblock->prev_next_info.le_next;
		}
		LIST_INSERT_BEFORE(&AllocMemBlocksList,firstblock, blockToInsert);
	}
}
//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================


struct MemBlock *alloc_block_FF(uint32 size)
{
	struct MemBlock* block ;
	uint32 sizee = LIST_SIZE(&FreeMemBlocksList);
		if(sizee == 0){return NULL;}
	LIST_FOREACH(block, &FreeMemBlocksList)
		{
			if ( block->size == size){
				struct MemBlock *new_block = block;
				LIST_REMOVE(&FreeMemBlocksList,block);
				return new_block;
			}

			else if (block->size > size){
				struct MemBlock *new_block = LIST_FIRST(&AvailableMemBlocksList);
				LIST_REMOVE(&AvailableMemBlocksList,new_block);
				new_block->size = size;
				new_block->sva= block->sva;
				block->sva = block->sva + size;
				block->size=block->size - size;

				return new_block;
			}
		}
	return NULL;
}


//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================

struct MemBlock *alloc_block_BF(uint32 size)
{
            struct MemBlock* block ;
		    struct MemBlock *Test_block=AvailableMemBlocksList.lh_first;
			struct MemBlock* new_block = NULL;
			struct MemBlock* new_free_block;
			unsigned int min =999999999;
			bool expression=0 ;
					LIST_FOREACH(block,&FreeMemBlocksList){
						if (block->size ==size ){
							LIST_REMOVE(&FreeMemBlocksList,block);
							return block;
						}else if (block->size > size && block->size <min){
							min = block->size ;
							new_free_block=block;
							expression =1;

						}
					}
					switch (expression){
					case 1:
						if (new_free_block!=NULL){
						Test_block->sva = new_free_block->sva;
						Test_block->size= size;
						new_free_block->sva = new_free_block->sva + size;
						new_free_block->size=new_free_block->size - size;
						new_block= Test_block;
						LIST_REMOVE(&AvailableMemBlocksList,new_block);
						return new_block;
					}
					}
					return NULL;
}


//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================

struct MemBlock *block=NULL;
struct MemBlock *Loop_function(uint32 size,struct MemBlock *newnode,struct MemBlock *block){
	if(newnode->size == size)
				{
					struct MemBlock*node = newnode;
					block =newnode;
					newnode = LIST_NEXT(newnode);
					LIST_REMOVE(&FreeMemBlocksList, node);
					return node;
				}
				else if(newnode->size > size)
				{
					struct MemBlock* ret = LIST_FIRST(&AvailableMemBlocksList);
					LIST_REMOVE(&AvailableMemBlocksList, ret);

					ret->sva = newnode->sva;
					ret->size = size;
	                block =ret;
					newnode->sva += size;
					newnode->size -= size;
					return ret;
				}
	return NULL;
}


struct MemBlock *alloc_block_NF(uint32 size)
	{
		struct MemBlock *newnode;
		struct MemBlock *newblock;
		struct MemBlock *newnode1;
		struct MemBlock *newnode2;
		if(block == NULL){
		LIST_FOREACH(newnode, &FreeMemBlocksList)
		{
			newblock = Loop_function(size,newnode,block);
			if(newblock==NULL){
				return NULL;
			}else{
				return newblock;
			}
		}
		}else if(block != NULL){
			struct MemBlock *newnode1;
			LIST_FOREACH(newnode1, &FreeMemBlocksList){
			if(newnode1->sva > block->sva){
				if(newnode1->size == size)
				{
					struct MemBlock*node = newnode1;
					block =newnode1;
					newnode1 = LIST_NEXT(newnode1);
					LIST_REMOVE(&FreeMemBlocksList, node);
					return node;
				}
				else if(newnode1->size > size)
				{
					struct MemBlock* ret = LIST_FIRST(&AvailableMemBlocksList);
					LIST_REMOVE(&AvailableMemBlocksList, ret);

					ret->sva = newnode1->sva;
					ret->size = size;
	                block =ret;
	                newnode1->sva += size;
	                newnode1->size -= size;
					return ret;
				}

			}
			}
			struct MemBlock *newnode2;
			LIST_FOREACH(newnode2, &FreeMemBlocksList){
			if(newnode2->sva < block->sva){
				if(newnode2->size == size)
				{
					struct MemBlock*node = newnode2;
					block =newnode2;
					newnode2 = LIST_NEXT(newnode2);
					LIST_REMOVE(&FreeMemBlocksList, node);
					return node;
				}
				else if(newnode2->size > size)
				{
					struct MemBlock* ret = LIST_FIRST(&AvailableMemBlocksList);
					LIST_REMOVE(&AvailableMemBlocksList, ret);

					ret->sva = newnode2->sva;
					ret->size = size;
	                block =ret;
	                newnode2->sva += size;
	                newnode2->size -= size;
					return ret;
				}

			}
			}

		}
		return NULL;
}




//===================================================
// [8] INSERT BLOCK (SORTED WITH MERGE) IN FREE LIST:
//===================================================
void insert_sorted_with_merge_freeList(struct MemBlock *blockToInsert)
{
	//ms3 edit
	struct MemBlock* element1= LIST_LAST(&FreeMemBlocksList);
	struct MemBlock* element2 = LIST_FIRST(&FreeMemBlocksList);
		//case 1 empty
		//no merge
		if(LIST_SIZE(&FreeMemBlocksList) == 0){
			LIST_INSERT_HEAD(&FreeMemBlocksList,blockToInsert);
		}
		else
		{
		// case 2 in tail
			if(blockToInsert->sva > element1->sva)
			{
				//merge in tail
				if(element1->sva + element1->size == blockToInsert->sva){
					uint32 totalsize = element1->size + blockToInsert->size;
					element1->size = totalsize;
					blockToInsert->sva = 0;
					blockToInsert->size =0;
					LIST_INSERT_HEAD(&AvailableMemBlocksList, blockToInsert);
				}
				//insert in tail with no merge
				else if(element1->sva + element1->size < blockToInsert->sva)
				{
					LIST_INSERT_TAIL(&FreeMemBlocksList, blockToInsert);
				}
			}
		//case in head
			else if(blockToInsert->sva < element2->sva)
			{
				//merge in head//////////////Don't remove element 2(first element)
				if(blockToInsert->sva + blockToInsert->size == element2->sva)
				{
					uint32 totalsize = element2->size + blockToInsert->size;
					element2->sva = blockToInsert->sva;
					element2->size =totalsize;
					blockToInsert->size = 0;
					blockToInsert->sva = 0;
					LIST_INSERT_HEAD(&FreeMemBlocksList, blockToInsert);
				}
				//insert in head with no merge
				else if(blockToInsert->sva + blockToInsert->size < element2->sva)
				{
					LIST_INSERT_HEAD(&FreeMemBlocksList, blockToInsert);
				}
			}
	    //case 3  in middle
			else{
					//////////
					struct MemBlock * element3;//next
					LIST_FOREACH(element3,&FreeMemBlocksList)
					{
						if(blockToInsert->sva<element3->sva)
						{
							element2=LIST_PREV(element3);//previous
							if(element2->sva+element2->size==blockToInsert->sva
									&&blockToInsert->size+blockToInsert->sva==element3->sva)
							{
								//Merge with previous and next
								uint32 totalsize = element2->size + blockToInsert->size + element3->size;
								element2->size = totalsize;
								blockToInsert->size=0;
								blockToInsert->sva=0;
								element3->size=0;
								element3->sva=0;
								LIST_INSERT_HEAD(&AvailableMemBlocksList,blockToInsert);
								LIST_REMOVE(&FreeMemBlocksList,element3);
								LIST_INSERT_HEAD(&AvailableMemBlocksList,element3);

							}
							else if(element2->sva+element2->size==blockToInsert->sva)
							{
								// merge with prev
								uint32 totalsize = element2->size+blockToInsert->size;
								element2->size = totalsize;
								blockToInsert->size=0;
								blockToInsert->sva=0;
								LIST_INSERT_HEAD(&AvailableMemBlocksList,blockToInsert);
							}
							else if(blockToInsert->size+blockToInsert->sva==element3->sva)
							{
								// merge with next
								uint32 totalsize = element3->size + blockToInsert->size;
								element3->size = totalsize;
								element3->sva= blockToInsert->sva;
								blockToInsert->size=0;
								blockToInsert->sva=0;
								LIST_INSERT_HEAD(&AvailableMemBlocksList,blockToInsert);
							}
							else
							{
								// no merge
								LIST_INSERT_BEFORE(&FreeMemBlocksList,element3,blockToInsert);
							}
							break;
						}

					}
				}
			}

}





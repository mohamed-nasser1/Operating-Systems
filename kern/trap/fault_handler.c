/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault
void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
//run hp 12 tmam
	uint32 *ptrrr_pageee_tableee = NULL;
	int flag_ws_isnotFull =0 ;
	int va_fault_with_round = ROUNDDOWN(fault_va,PAGE_SIZE);
	struct FrameInfo *ptrrr_frameee_infooo;
	uint32 cur_size = env_page_ws_get_size(curenv);
	for (int i = 0; i<curenv->page_WS_max_size; i++){
		if (env_page_ws_is_entry_empty(curenv, i))
		{
			curenv->page_last_WS_index = i;
			break;
		}
       }
	if (cur_size < curenv->page_WS_max_size){
		flag_ws_isnotFull=1;
	}
	else{
		flag_ws_isnotFull=0;
	}
	// placement
	if (flag_ws_isnotFull==1){
		allocate_frame(&ptrrr_frameee_infooo);
		map_frame(curenv->env_page_directory, ptrrr_frameee_infooo, fault_va, PERM_USER|PERM_WRITEABLE);
		int mawgod_wla_la = pf_read_env_page( curenv, (void*)fault_va);
		if (mawgod_wla_la == E_PAGE_NOT_EXIST_IN_PF)
		{
        	if ((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX ) ||
				(fault_va >= USER_HEAP_MAX && fault_va < USTACKTOP) ){
        		allocate_frame(&ptrrr_frameee_infooo);
        		map_frame(curenv->env_page_directory, ptrrr_frameee_infooo, fault_va, PERM_USER|PERM_WRITEABLE);
        		env_page_ws_set_entry(curenv, curenv->page_last_WS_index , va_fault_with_round) ;
        		//curenv->page_last_WS_index ++ ;
			}
			else{
				panic("ILLEGAL MEMORY ACCESS ");
				}
		}
		env_page_ws_set_entry(curenv, curenv->page_last_WS_index , va_fault_with_round) ;
		curenv->page_last_WS_index ++ ;
		if(curenv->page_last_WS_index == curenv->page_WS_max_size)
			curenv->page_last_WS_index=0;
    }
	//replacement
	else if (flag_ws_isnotFull==0){
		for(int i=0 ; i<= curenv->page_WS_max_size ; i++){
			uint32 va_index=curenv->ptr_pageWorkingSet[curenv->page_last_WS_index].virtual_address;
        	int perm_index = pt_get_page_permissions(curenv->env_page_directory,va_index);
        	int xyz=get_page_table(curenv->env_page_directory, va_index, &ptrrr_pageee_tableee);
        	if (xyz== TABLE_IN_MEMORY){
        	if((ptrrr_pageee_tableee!=NULL)&&(perm_index & PERM_USED) != PERM_USED) {
				if ((perm_index & PERM_MODIFIED) == PERM_MODIFIED){
					struct FrameInfo* MPFI = get_frame_info(curenv->env_page_directory, va_index,&ptrrr_pageee_tableee);
					pf_update_env_page(curenv,va_index,MPFI );
				}
				    env_page_ws_clear_entry(curenv,curenv->page_last_WS_index);
					unmap_frame(curenv->env_page_directory, va_index);
					//placement
					allocate_frame(&ptrrr_frameee_infooo);
					map_frame(curenv->env_page_directory, ptrrr_frameee_infooo, fault_va, PERM_USER|PERM_WRITEABLE);
					int mawgod_wla_la = pf_read_env_page( curenv, (void*)fault_va);
					if (mawgod_wla_la == E_PAGE_NOT_EXIST_IN_PF)
						{
						if ((fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX ) ||
							(fault_va >= USER_HEAP_MAX && fault_va < USTACKTOP) ){
							env_page_ws_set_entry(curenv, curenv->page_last_WS_index , va_fault_with_round) ;
						}
						 else{
								panic("ILLEGAL MEMORY ACCESS ");
								}
						 }
					env_page_ws_set_entry(curenv, curenv->page_last_WS_index , va_fault_with_round) ;
					curenv->page_last_WS_index ++ ;
					if(curenv->page_last_WS_index == curenv->page_WS_max_size){
						curenv->page_last_WS_index=0;
					}
					break;

	           }
        	else if ((ptrrr_pageee_tableee!=NULL)&&(perm_index & PERM_USED) == PERM_USED){
				// pt_set_page_permissions(curenv->env_page_directory, va_index,0,PERM_USED);
        		ptrrr_pageee_tableee[PTX(va_index)]=( ptrrr_pageee_tableee[PTX(va_index)] &(~PERM_USED));
				curenv->page_last_WS_index ++;
				 if(curenv->page_last_WS_index == curenv->page_WS_max_size){
					 curenv->page_last_WS_index=0;}
		    }
		}
	}

	}
  return ;
}


void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	// Write your code here, remove the panic and write your code
	panic("__page_fault_handler_with_buffering() is not implemented yet...!!");


}

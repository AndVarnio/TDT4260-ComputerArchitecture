/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"
#include <map>

//Struct for the Reference Prediction Table
/*State:
1=initialize
2=transient
3=steady
4=no-prediction*/
struct RPTable{
  Addr addrPc, addrPrevMem;
  int stride, state;
};

//Map with the address of the instruction as key and a RPT as element
static std::map<Addr,RPTable> mapRPT;
void prefetch_init(void)
{
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
}

void prefetch_now(AccessStat stat){
  //Checking if the new instruction adress is in the table, if not is is added.
  //If it is there the table is updated
  //Case A.1.
  if(mapRPT.end() == mapRPT.find(stat.pc)){
    mapRPT[stat.pc].addrPc = stat.pc;
    mapRPT[stat.pc].addrPrevMem = 0;
    mapRPT[stat.pc].stride = 0;
    mapRPT[stat.pc].state = 1;
  }
  //Case A.2.
  else{
    //a
    if(mapRPT[stat.pc].stride != stat.mem_addr - mapRPT[stat.pc].addrPrevMem && mapRPT[stat.pc].state == 1){

      mapRPT[stat.pc].stride = stat.mem_addr - mapRPT[stat.pc].addrPrevMem;
      mapRPT[stat.pc].addrPrevMem = stat.mem_addr;
      mapRPT[stat.pc].state = 2;
    }
    //b
    else if(mapRPT[stat.pc].stride == stat.mem_addr - mapRPT[stat.pc].addrPrevMem){
      mapRPT[stat.pc].addrPrevMem = stat.mem_addr;
      mapRPT[stat.pc].state = 3;
    }
    //c
    else if(mapRPT[stat.pc].stride != stat.mem_addr - mapRPT[stat.pc].addrPrevMem && mapRPT[stat.pc].state == 3){
      mapRPT[stat.pc].addrPrevMem = stat.mem_addr;
      mapRPT[stat.pc].state = 1;
    }
    //d
    else if(mapRPT[stat.pc].stride != stat.mem_addr - mapRPT[stat.pc].addrPrevMem && mapRPT[stat.pc].state == 2){
      mapRPT[stat.pc].stride = stat.mem_addr - mapRPT[stat.pc].addrPrevMem;
      mapRPT[stat.pc].addrPrevMem = stat.mem_addr;

      mapRPT[stat.pc].state = 4;
    }
    //e
    else if(mapRPT[stat.pc].stride == stat.mem_addr - mapRPT[stat.pc].addrPrevMem && mapRPT[stat.pc].state == 4){
      mapRPT[stat.pc].addrPrevMem = stat.mem_addr;
      mapRPT[stat.pc].state = 2;
    }
    //f
    else if(mapRPT[stat.pc].stride != stat.mem_addr - mapRPT[stat.pc].addrPrevMem && mapRPT[stat.pc].state == 4){

      mapRPT[stat.pc].stride = stat.mem_addr - mapRPT[stat.pc].addrPrevMem;
      mapRPT[stat.pc].addrPrevMem = stat.mem_addr;
    }
    else{
      mapRPT[stat.pc].state = 1;
    }

    if(!in_cache(stat.mem_addr+(uint64_t)mapRPT[stat.pc].stride) && mapRPT[stat.pc].state == 3){
      issue_prefetch(stat.mem_addr+(uint64_t)mapRPT[stat.pc].stride);
      DPRINTF(HWPrefetch, "Prefetching\n");
    }
  }
}

void prefetch_access(AccessStat stat)
{

  //if(stat.miss)
	//{
		//stat.mem_addr &= -BLOCK_SIZE;
		prefetch_now(stat);
	//}

}

void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */
}

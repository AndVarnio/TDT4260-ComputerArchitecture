/*
 * A sample prefetcher which does sequential one-block lookahead.
 * This means that the prefetcher fetches the next block _after_ the one that
 * was just accessed. It also ignores requests to blocks already in the cache.
 */

#include "interface.hh"
#include <map>
#include <vector>

//Struct for the Delta Table
const int numbDelt = 4;
struct deltaTable{
  Addr lastAddr, lastPref;
  int ptrDelta;
  int delta[numbDelt];
};

//Map with the address of the instruction as key and a deltaTable as element
static std::map<Addr,deltaTable> mapDelta;
void prefetch_init(void)
{
    /* Called before any calls to prefetch_access. */
    /* This is the place to initialize data structures. */

    DPRINTF(HWPrefetch, "Initialized sequential-on-access prefetcher\n");
}

void prefetch_access(AccessStat stat)
{

  //Creates new entry
  //stat.mem_addr &= -BLOCK_SIZE;
  std::vector<int> vectCandidates;
  if(mapDelta.end() == mapDelta.find(stat.pc)){
    mapDelta[stat.pc].lastAddr = stat.pc;
    mapDelta[stat.pc].lastPref = (uint64_t)0;
    mapDelta[stat.pc].ptrDelta = 0;
    for(int i = 0; i < numbDelt; i++){
        mapDelta[stat.pc].delta[i] = 0;
    }
  }
  // Update entry
  else{
    // Moves the deltas in table to the right to make room for new delta
    if(stat.mem_addr-mapDelta[stat.pc].lastAddr>0){
      for(int i = numbDelt-1; i > 0; i--){
          mapDelta[stat.pc].delta[i] = mapDelta[stat.pc].delta[i-1];
      }
      //If delta too big set 0 to indicate overflow if not set new delta to last memory access - new memory access
      if(stat.mem_addr-mapDelta[stat.pc].lastAddr>2147483647){
        mapDelta[stat.pc].delta[0] = 0;
      }
      else{
        mapDelta[stat.pc].delta[0] = stat.mem_addr-mapDelta[stat.pc].lastAddr;
      }
      //Continue if no overflow
      if(mapDelta[stat.pc].delta[0]!=0 && mapDelta[stat.pc].delta[1]!=0){
        bool matchFound = false;
        int n = 1;
        //Iterate backwards in delta array untill a matching pair is encountered
        while(matchFound == false && n < numbDelt-1){
          n++;
          if(mapDelta[stat.pc].delta[0]==mapDelta[stat.pc].delta[n] && mapDelta[stat.pc].delta[1]==mapDelta[stat.pc].delta[n])
            matchFound = true;
        }
        //Iterate from matching pair toward head(right) and add all the deltas to this memory access in vectorCandidates
        // if this memory access + delta = last prefetch clear whole vector
        for(int i=0; i<n; i++){
          if(mapDelta[stat.pc].delta[n-i-1]==0)break;
          vectCandidates.push_back(stat.mem_addr+mapDelta[stat.pc].delta[n-i-1]);
          if(mapDelta[stat.pc].delta[n-i-1]==mapDelta[stat.pc].lastPref) vectCandidates.clear();
        }
}
  //Prefetch everything in vector if it dosent exceed physical memory
  if(vectCandidates.size()>0){
    for(int i=0; i<vectCandidates.size(); i++){
      if(stat.mem_addr+(uint64_t)(mapDelta[stat.pc].delta[i] < MAX_PHYS_MEM_ADDR)){
        issue_prefetch(stat.mem_addr+(uint64_t)(mapDelta[stat.pc].delta[i]));
        mapDelta[stat.pc].lastPref = stat.mem_addr+(uint64_t)(mapDelta[stat.pc].delta[i]);
      }
    }
  }

  //Update last memory access
  mapDelta[stat.pc].lastAddr = stat.mem_addr;
  }
  }
}

void prefetch_complete(Addr addr) {
    /*
     * Called when a block requested by the prefetcher has been loaded.
     */
}

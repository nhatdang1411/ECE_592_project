#include "mem/cache/cache_1.hh"

//Check prefetcher for hit and miss
pref_t cache::prefetch_access(uint64_t addr){
  pref_t pref_s;
  pref_s.pref_index = 0;
  uint64_t pref = pref_n;
  uint64_t pref_lru_c = pref_n;
  for (int i = pref_n-1; i>=0; i--) {
    if (pref_v[i]){
      for (int ii = pref_m-1; ii >= 0; ii--){
        if ((pref_addr[i][ii] == (addr>>block_offset)) && (pref_lru_c > pref_lru[i])) {
          pref = i;
          pref_s.pref_index = ii;
          pref_lru_c = pref_lru[i];
        }
      }
    }
  }
  pref_s.pref = pref;
  return pref_s;
}
//Select victim prefetcher for update
uint64_t cache::prefetch_victim(){
  for (int i=pref_n-1; i>=0; i--) {
    if (pref_lru[i] == (pref_n-1))
      return i;
  }
  return 0;
}

//Update lru for prefetcher
void cache::prefetch_lru_update(uint64_t pref){
  for (int i=pref_n-1; i>=0; i--) {
    if (pref_lru[i]<pref_lru[pref]){
      pref_lru[i]++;
    }
  }
  pref_lru[pref]=0;
}
//Update prefetcher when miss happens
void cache::prefetch_update_miss(uint64_t addr, uint32_t pref){
  for (uint64_t i=0; i<=pref_m-1; i++){
    pref_addr[pref][i] = (addr >> block_offset)+i+1;
  }
  prefetch+=pref_m;
  pref_v[pref] = 1;
  hd_ptr[pref] = 0;
  //print_prefetch_content();
}

//Update prefetcher when hit
void cache::prefetch_update_hit(uint64_t addr, uint32_t pref, uint32_t pref_index){
  uint64_t ptr = hd_ptr[pref];
  //printf("%d %d ", ptr, pref_index);
  //if (ptr == (pref_m-1)) {ptr = 0;}
  if (hd_ptr[pref] == pref_index) {
    pref_addr[pref][ptr]=(addr >> block_offset)+pref_m;
    hd_ptr[pref] = (ptr == (pref_m-1)) ? 0 :pref_index+1 ;
    prefetch+=1;
  } else if (hd_ptr[pref] > pref_index) {
    uint64_t k = 0;
    for (uint64_t i = 0; i <= pref_m-hd_ptr[pref]+pref_index; i++) {
      if (ptr+k<pref_m){
        pref_addr[pref][ptr+k] = (addr >> block_offset)+hd_ptr[pref]-pref_index+i;
      } else {
        ptr = 0;
        k = 0;
        pref_addr[pref][ptr+k] = (addr >> block_offset)+hd_ptr[pref]-pref_index+i;
      }
      k++;
      prefetch+=1;
    }
    hd_ptr[pref] = pref_index+1;

  } else {
    for (uint64_t i=0; i <= pref_index-hd_ptr[pref];i++){
          pref_addr[pref][ptr+i] = (addr >> block_offset)+pref_m-pref_index+hd_ptr[pref]+i;
          prefetch++;
    }
    if (pref_index < (pref_m-1))
      hd_ptr[pref] = pref_index+1;
    else
      hd_ptr[pref] = 0;
  }
  //print_prefetch_content();

}

//Fetch the new address
bool cache::read_access(uint64_t addr) {
  read++;
  uint64_t tag_addr = addr >> (index + block_offset);
  uint64_t index_addr = (addr >> block_offset) - (tag_addr << index);
  cache_access_t acc = cache_access(index_addr, tag_addr);
  pref_t pref_s = prefetch_access(addr);
  uint64_t pref_victim = prefetch_victim();
  int victim = check_victim_block(index_addr);
  uint64_t victim_addr = get_victim_addr(index_addr);

  if (!acc.hit) {
    read_miss++;
      if (victim) {
        writeback++;
        if (next != NULL) {
          next->write_access(victim_addr);
          next->read_access(addr);
        } else {
        }
      } else {
        if (next != NULL) {
          next->read_access(addr);
        } else {


        }
      }

      //update_cache(addr);
  } else {
    update_lru(index_addr, acc.way);
  };

  if ((next == NULL) && (pref_n != 0)) {
    if (pref_s.pref == pref_n) {
      if (!acc.hit) {
        prefetch_lru_update(pref_victim);
        prefetch_update_miss(addr, pref_victim);
       // prefetch_lru_update(pref_victim);
      }
    } else {
      if(!acc.hit) {read_miss--;}
      prefetch_lru_update(pref_s.pref);
      prefetch_update_hit(addr, pref_s.pref, pref_s.pref_index);
      //prefetch_lru_update(pref_s.pref);
    }
  }
  return acc.hit;
};


void cache::write_access(uint64_t addr) {
  write++;
  uint64_t tag_addr = addr >> (index + block_offset);
  uint64_t index_addr = (addr >> block_offset) - (tag_addr <<  index);
  cache_access_t acc = cache_access(index_addr, tag_addr);
  pref_t pref_s = prefetch_access(addr);
  uint64_t pref_victim = prefetch_victim();
  int victim = check_victim_block(index_addr);
  uint64_t victim_addr = get_victim_addr(index_addr);

  if (!acc.hit) {
    write_miss++;
      if (victim) {
        writeback++;
        if (next != NULL) {
          next->write_access(victim_addr);
          next->read_access(addr);
        } else {

        }
      } else {
        if (next != NULL) {
          next->read_access(addr);
        }
      }

      update_cache(addr);
  } else {
    update_lru(index_addr, acc.way);
  }
  set_dirty_bit(index_addr, tag_addr);

  if ((next == NULL) && (pref_n != 0)) {
    if (pref_s.pref == pref_n) {
      if (!acc.hit) {
       prefetch_lru_update(pref_victim);
        prefetch_update_miss(addr, pref_victim);
       // prefetch_lru_update(pref_victim);
      }
    } else {
      //assert(1);
      if(!acc.hit) {write_miss--;}
      prefetch_lru_update(pref_s.pref);
      prefetch_update_hit(addr, pref_s.pref, pref_s.pref_index);
     // prefetch_lru_update(pref_s.pref);
    }
  }
};
void cache::set_dirty_bit(uint64_t index_addr, uint32_t tag_addr){
  for (int i = assoc-1; i>=0; i--){
    if (valid[index_addr][i] && (tag[index_addr][i] == tag_addr)){
      dirty[index_addr][i]=1;
      return;
    }
  }
};

cache_access_t cache::cache_access(const uint64_t index_addr, const uint32_t tag_addr){
  cache_access_t acc;
  acc.hit = 0;
  acc.way = 0;
  for (int i = assoc-1; i>=0; i--){
    if (valid[index_addr][i] && (tag[index_addr][i] == tag_addr)){
      //update_lru(index_addr, i);
      acc.hit = 1;
      acc.way = i;
      return acc;
    }
  }
  return acc;

};

void cache::update_cache( uint64_t addr){
  uint64_t tag_addr = addr >> (index + block_offset);
  uint64_t index_addr = (addr >> block_offset) - (tag_addr <<  index);
  for (int i = assoc-1; i>=0; i--){
    if (lru[index_addr][i] == (assoc-1)){
      tag[index_addr][i] = tag_addr;
      valid[index_addr][i] = 1;
      dirty[index_addr][i] = 0;
      update_lru(index_addr, i);
      return;
    }
  }
};

void cache::update_lru(uint64_t index_addr, uint32_t assoc_addr){
  for (uint64_t i = 0; i<=assoc-1; i++){
    if ((i != assoc_addr) && (lru[index_addr][assoc_addr] > lru[index_addr][i])){
      lru[index_addr][i]++;
    }
  };

  lru[index_addr][assoc_addr] = 0;
};

int cache::check_victim_block(uint64_t index_addr){
  for (int i = assoc-1; i >= 0; i--){
    if ((lru[index_addr][i] == (assoc -1)) && (dirty[index_addr][i]) && (valid[index_addr][i])){
      return 1;
    }
  }
  return 0;
};

uint64_t cache::get_victim_addr(uint32_t index_addr){
  for (int i = assoc-1; i >= 0; i--){
    if ((lru[index_addr][i] == (assoc -1)) && (dirty[index_addr][i])){
      return ((tag[index_addr][i]<<(index+block_offset))+ (index_addr<<block_offset));
    }
  }
  return 0;
};

void cache::print_cache_content(){
  for (uint64_t i=0; i<set; i++){
    printf("set%7d: ",i);
    uint64_t k = 0;
    while (k < assoc){
     for (uint64_t ii=0; ii<=assoc-1; ii++){
      if (lru[i][ii] == k) {
          uint64_t addr = (tag[i][ii]);
          printf("%8x ",addr);
         // std::fflush(stdout);
        if (dirty[i][ii] == 1) {
          printf("D");
        } else {
          printf(" ");
        }
      }
     }
     k++;
    }
    printf("\n");
  }
};

void cache::print_internal_cache(){
  cache::print_cache_content();
}

void cache::print_prefetch_content(){
  uint64_t m = 0;
  while (m < pref_n){
    for (uint64_t i=0; i<pref_n; i++){
      if (pref_lru[i] == m){
        uint64_t k = hd_ptr[i];
        //printf("SB: ");
          for (uint64_t ii=0; ii<pref_m; ii++){
            printf("%8x ",pref_addr[i][k]);
            if (k == pref_m-1){
              k = 0;
            } else {
              k++;
            }
          }
        printf("\n");
        }
    }
    m++;
  }
}

void cache::print_internal_prefetcher(){
  cache::print_prefetch_content();
}

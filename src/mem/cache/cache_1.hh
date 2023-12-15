#ifndef __MEM_CACHE_1_HH__
#define __MEM_CACHE_1_HH__
#include <stdio.h>
#include <stdlib.h>
#include <math.h>       /* log2 */
#include <inttypes.h>
#include <iostream>
#include "sim.hh"
#include <cassert>

class cache {
  public:
      uint64_t size, blocksize, pref_n, pref_m, assoc;
      uint64_t read{0}, read_miss{0}, write{0}, write_miss{0}, writeback{0},
               prefetch{0};
      double miss_rate;
      cache* next;

      void write_access(uint64_t addr);
      bool read_access(uint64_t addr);
      void print_internal_cache();
      void print_cache_content();
      void print_internal_prefetcher();
      void update_cache(uint64_t addr);

      cache(uint64_t size, uint32_t blocksize, uint32_t assoc,
          uint64_t pref_n, uint32_t pref_m){
        this->size = size;
        this->blocksize = blocksize;
        this->pref_n = pref_n;
        this->pref_m = pref_m;
        this->assoc = assoc;
        if (this->size != 0){
          set = size/(blocksize * assoc);
        } else {
          set = 0;
        }
        block_offset = log2(blocksize);
        index = log2(set);

        tag = new uint64_t*[set];
        valid = new uint64_t*[set];
        lru = new uint64_t*[set];
        dirty = new uint64_t*[set];
        if (pref_n>0) {
          pref_addr = new uint64_t*[pref_n];
          pref_lru = new uint64_t[pref_n];
          hd_ptr = new uint64_t[pref_n];
          pref_v = new uint64_t[pref_n];
          for (int i = pref_n-1; i >=0 ; i--){
            pref_addr[i] = new uint64_t[pref_m];
            pref_lru[i] = i;
            pref_v[i] = 0;
          }
        }

        for (int i = set-1; i >= 0; i--){
          tag[i] = new uint64_t[assoc];
          valid[i] = new uint64_t[assoc];
          lru[i] = new uint64_t[assoc];
          dirty[i] = new uint64_t[assoc];
          for (int ii =  assoc-1; ii >= 0; ii--){
            valid[i][ii] = 0;
            lru[i][ii] = ii;
          }
        }
      }

      ~cache(){

        for (int i = set-1; i >= 0; i--){
          delete [] tag[i];
          delete [] valid[i];
          delete [] dirty[i];
          delete [] lru[i];
        }

        for (int i = pref_n-1; i >= 0; i--){
          delete [] pref_addr[i];
        }
        delete []tag;
        delete []valid;
        delete []lru;
        delete []dirty;
        delete []pref_addr;
        delete pref_lru;
        delete hd_ptr;
        delete pref_v;
      }

  private:

      //Attribute
      uint64_t set;
      uint64_t index;
      uint64_t block_offset;

      uint64_t** tag;
      uint64_t** valid;
      uint64_t** lru;
      uint64_t** dirty;
      uint64_t** pref_addr;
      uint64_t* pref_lru;
      uint64_t* hd_ptr;
      uint64_t* pref_v;
      //Method
      void print_prefetch_content();
      pref_t prefetch_access(uint64_t addr);
      uint64_t prefetch_victim();
      void prefetch_lru_update(uint64_t pref);
      void prefetch_update_miss(uint64_t addr, uint32_t pref);
      void prefetch_update_hit(uint64_t addr, uint32_t pref, uint32_t pref_index);
      cache_access_t cache_access(const uint64_t index_addr, const uint32_t tag_addr);
      void update_lru(uint64_t index_addr, uint32_t assoc_addr);
      int check_victim_block(uint64_t index_addr);
      uint64_t get_victim_addr(uint32_t index_addr);
      void set_dirty_bit(uint64_t index_addr, uint32_t tag_addr);
      //Stats


};
#endif

/*
 * ClauseAllocator.h
 *
 *  Created on: Feb 20, 2017
 *      Author: markus
 */

#ifndef SRC_CANDY_CORE_CLAUSEALLOCATOR_H_
#define SRC_CANDY_CORE_CLAUSEALLOCATOR_H_

#include <vector>
#include <array>
#include <cstdint>
#include <cstring>
#include <numeric>

#include <candy/core/Clause.h>
#include <candy/core/Statistics.h>

#define NUMBER_OF_POOLS 500
#define DEFAULT_POOL_SIZE 70
#define POOL_GROWTH_FACTOR 2
#define XXL_POOL_INDEX 500u
#define XXL_POOL_ONE_SIZE 1020
#define REVAMPABLE_PAGES_MAX_SIZE 6

namespace Candy {

class ClauseAllocator {

public:
    ClauseAllocator() : pools(), pages(), pages_nelem() {
        for (auto& pool : pools) {
            pool.reserve(DEFAULT_POOL_SIZE);
        }
    }

    ~ClauseAllocator() {
        for (auto pages : this->pages) {
            for (char* page : pages) {
                free(page);
            }
        }
    }

    ClauseAllocator(ClauseAllocator const&) = delete;
    void operator=(ClauseAllocator const&)  = delete;

    /**
     * Preallocate additional storage for given list of clauses.
     */
    void announceClauses(const std::vector<std::vector<Lit>*>& problem);
    void announceClauses(const std::vector<Clause*>& clauses);

    inline void* allocate(unsigned int length) {
        if (length <= XXL_POOL_ONE_SIZE) {
            unsigned int index = getPoolIndex(length);
            std::vector<void*>& pool = pools[index];

            Statistics::getInstance().allocatorPoolAlloc(index);

            if (pool.size() == 0) {
                pool.reserve(pool.capacity() * POOL_GROWTH_FACTOR);
                fillPool(index);
            }

            void* clause = pool.back();
            pool.pop_back();
            return clause;
        }
        else {
            Statistics::getInstance().allocatorPoolAlloc(501);
            return malloc(clauseBytes(length));
        }
    }

    inline void deallocate(Clause* clause) {
        if (clause->size() <= XXL_POOL_ONE_SIZE) {
            unsigned int index = getPoolIndex(clause->size());
            clause->activity() = 0;
            pools[index].push_back(clause);
            Statistics::getInstance().allocatorPoolDealloc(index);
        }
        else {
            free((void*)clause);
            Statistics::getInstance().allocatorPoolDealloc(501);
        }
    }

    inline void prefetchPage(unsigned int clause_size) {
#ifdef SOFTWARE_PREFETCHING
        assert(clause_size > 2);
        assert(clause_size <= REVAMPABLE_PAGES_MAX_SIZE);
        unsigned int index = getPoolIndex(clause_size);
        if (pages[index].size() > 0) {
            char* start = pages[index][0];
            char* end = start + clauseBytes(clause_size) * pages_nelem[index][0];
            for(char* adr = start; adr < end; adr += 64) {
                //_mm_prefetch(adr, _MM_HINT_T1);
                __builtin_prefetch(adr, 0, 2);
            };
        }
#endif
    }

    std::vector<Clause*> revampPages(size_t i);

private:
    std::array<std::vector<void*>, NUMBER_OF_POOLS+1> pools;

    std::array<std::vector<char*>, REVAMPABLE_PAGES_MAX_SIZE+1> pages;
    std::array<std::vector<size_t>, REVAMPABLE_PAGES_MAX_SIZE> pages_nelem;

    void fillPool(unsigned int index);
    void preallocateStorage(std::array<unsigned int, 501>& nclauses);

    inline unsigned int clauseBytes(unsigned int length) {
        return (sizeof(Clause) + sizeof(Lit) * (length-1));
    }

    inline unsigned int getPoolIndex(unsigned int size) const {
        return std::min(size-1, XXL_POOL_INDEX);
    }

    template <unsigned int N> struct SortHelperClause {
        uint16_t length;
        uint16_t header;
        float act;
        Lit literals[N];
    };

    template <unsigned int N> std::vector<Clause*> revampPages();

};

} /* namespace Candy */

#endif /* SRC_CANDY_CORE_CLAUSEALLOCATOR_H_ */

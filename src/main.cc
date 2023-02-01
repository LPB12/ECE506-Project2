/*******************************************************
                          main.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include <fstream>
using namespace std;

#include "cache.h"

int main(int argc, char *argv[])
{
    
    ifstream fin;
    FILE * pFile;

    if(argv[1] == NULL){
         printf("input format: ");
         printf("./smp_cache <cache_size> <assoc> <block_size> <num_processors> <protocol> <trace_file> \n");
         exit(0);
        }

    ulong cache_size     = atoi(argv[1]);
    ulong cache_assoc    = atoi(argv[2]);
    ulong blk_size       = atoi(argv[3]);
    ulong num_processors = atoi(argv[4]);
    ulong protocol       = atoi(argv[5]); /* 0:MSI 1:MSI BusUpgr 2:MESI 3:MESI Snoop FIlter */
    char *fname        = (char *) malloc(20);
    fname              = argv[6];
    uint32_t transac;
    bool shared;
  
    

 
    printf("===== 506 Coherence Simulator Configuration =====\n");
    // print out simulator configuration here
    printf("L1_SIZE: %d\n", (int)cache_size);
    printf("L1_ASSOC: %d\n", (int)cache_assoc);
    printf("L1_BLOCKSIZE: %d\n", (int)blk_size);
    printf("NUMBER OF PROCESSORS: %d\n", (int)num_processors);
    switch(protocol)
    {
        case MSI:
            printf("COHERENCE PROTOCOL: MSI\n");
            break;
        case MSIPLUS:
            printf("COHERENCE PROTOCOL: MSI BusUpgr\n");
            break;
        case MESI:
            printf("COHERENCE PROTOCOL: MESI\n");
            break;
        case MESIFILTER:
            printf("COHERENCE PROTOCOL: MESI Filter BusNOP\n");
            break;
        default: break;
    }
    printf("TRACE FILE: %s\n", fname);
    // Using pointers so that we can use inheritance */
    Cache** cacheArray = (Cache **) malloc(num_processors * sizeof(Cache));
    for(ulong i = 0; i < num_processors; i++) {
        
            cacheArray[i] = new Cache(cache_size, cache_assoc, blk_size, protocol);
        
    }
    

    pFile = fopen (fname,"r");
    if(pFile == 0)
    {   
        printf("Trace file problem\n");
        exit(0);
    }
    
    ulong proc;
    char op;
    ulong addr;

    int line = 1;
    while(fscanf(pFile, "%lu %c %lx", &proc, &op, &addr) != EOF)
    {
#ifdef _DEBUG
        printf("%d\n", line);
#endif
        shared = false;

        transac = cacheArray[proc]->Access(addr, op, protocol);
        if(transac != NOSIG)
        {
            for(uint32_t  i = 0; i < num_processors; i++)
            {
                if(i != proc)
                {
                     shared |= cacheArray[i]->Snoop(transac, addr, protocol);
                }

            }

        }

        if((protocol == MESI || protocol == MESIFILTER) && (transac == BUS_RD || transac == BUS_RDX))
        {
            cacheArray[proc] -> secondRound(shared, addr, transac);
        }
        
        
        // propagate request down through memory hierarchy
        // by calling cachesArray[processor#]->Access(...)

        line++;
    }

    fclose(pFile);

    for(uint32_t i = 0; i < num_processors; i++){
        cacheArray[i]->printStats(i, protocol);

    }
    //********************************//
    //print out all caches' statistics //
    //********************************//
    
}

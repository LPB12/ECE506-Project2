/*******************************************************
                          cache.cc
********************************************************/

#include <stdlib.h>
#include <assert.h>
#include "cache.h"
using namespace std;

Cache::Cache(int s,int a,int b, ulong protocol)
{
   ulong i, j;
   reads = readMisses = writes = 0; 
   writeMisses = writeBacks = currentCycle = 0;
   flushes = cache2cache = numBusRdX = numBusUp = numMemTransac = numInter = numInval = usefulSnoop = wastedSnoop = 0;
   filterSnoop = 0;

   size       = (ulong)(s);
   lineSize   = (ulong)(b);
   assoc      = (ulong)(a);   
   sets       = (ulong)((s/b)/a);
   numLines   = (ulong)(s/b);
   log2Sets   = (ulong)(log2(sets));   
   log2Blk    = (ulong)(log2(b));   
  
   //*******************//
   //initialize your counters here//
   //*******************//
 
   tagMask =0;
   for(i=0;i<log2Sets;i++)
   {
      tagMask <<= 1;
      tagMask |= 1;
   }
   
   /**create a two dimentional cache, sized as cache[sets][assoc]**/ 
   cache = new cacheLine*[sets];
   for(i=0; i<sets; i++)
   {
      cache[i] = new cacheLine[assoc];
      for(j=0; j<assoc; j++) 
      {
         cache[i][j].invalidate();
      }
   }      

   if(protocol == MESIFILTER )
   {
      historyFilter = (Cache *) malloc(sizeof(Cache));
      historyFilter = new Cache(16 * b, 1, b, MSI);
   }
   
}

/**you might add other parameters to Access()
since this function is an entry point 
to the memory hierarchy (i.e. caches)**/
uint32_t Cache::Access(ulong addr,uchar op, ulong protocol)
{
   currentCycle++;/*per cache global counter to maintain LRU order 
                    among cache ways, updated on every cache access*/
         
   if(op == 'w') writes++;
   else          reads++;
   
   cacheLine * line = findLine(addr);

   if(protocol == MESIFILTER)
   {
      if(historyFilter->findLine(addr))
      {
         historyFilter->removeFromCache(addr);
      }
   }


   if(line == NULL)/*miss*/
   {
      if(op == 'w') writeMisses++;
      else readMisses++;

      cacheLine *newline = fillLine(addr);
      if(op == 'w') 
      {
         newline->setFlags(MODIFIED);    
         numBusRdX++;
         numMemTransac++;
         return BUS_RDX;
         
      }
      else
      {
         newline->setFlags(SHARED);
         numMemTransac++;
         return BUS_RD;
      }
   }
   else
   {
      /**since it's a hit, update LRU and update dirty flag**/
      updateLRU(line);
      if(op == 'w')
      {
         switch(protocol)
         {
            case MSI:  
               switch(line->getFlags())
               {
                  case MODIFIED:  
                     return NOSIG;
                     break;
                  case SHARED:
                     line->setFlags(MODIFIED);
                     numBusRdX++;
                     numMemTransac++;
                     return BUS_RDX;
                     break;
                  case INVALID:
                     line->setFlags(MODIFIED);
                     numBusRdX++;
                     numMemTransac++;
                     return BUS_RDX;
                     break;     
                  default: break;
               }
               break;
            case MSIPLUS:
               switch(line->getFlags())
               {
                  case MODIFIED:
                     return NOSIG;
                     break;  
                  case SHARED:
                     line->setFlags(MODIFIED);
                     numBusUp++;
                     return BUS_UPGR;
                     break;
                  case INVALID:
                     line->setFlags(MODIFIED);
                     numBusRdX++;
                     numMemTransac++;
                     return BUS_RDX;
                     break;     
                  default: break;
               }
               break;
            case MESI:
               switch(line->getFlags())
               {
                  case MODIFIED:  
                     return NOSIG;
                     break;
                  case EXCLUSIVE:
                     line->setFlags(MODIFIED);
                     return NOSIG;
                     break;
                  case SHARED:
                     line->setFlags(MODIFIED);
                     numBusUp++;
                     return BUS_UPGR;
                     break;
                  case INVALID:
                     line->setFlags(MODIFIED);
                     numBusRdX++;
                     numMemTransac++; 
                     return BUS_RDX;
                     break;     
                  default: break;
               }
               break;
            case MESIFILTER:
               switch(line->getFlags())
               {
                  case MODIFIED:  
                     return NOSIG;
                     break;
                  case EXCLUSIVE:
                     line->setFlags(MODIFIED);
                     return NOSIG;
                     break;
                  case SHARED:
                     line->setFlags(MODIFIED);
                     numBusUp++;
                     return BUS_UPGR;
                     break;
                  case INVALID:
                     line->setFlags(MODIFIED);
                     numBusRdX++;
                     numMemTransac++;
                     return BUS_RDX;
                     break;     
                  default: break;
               }
               break;      
            default: break;
         }
      } 
      else
      {
         switch(protocol)
         {
            case MSI:  
               switch(line->getFlags())
               {
                  case MODIFIED:  
                     return NOSIG;
                     break;
                  case SHARED:
                     return NOSIG;
                     break;
                  case INVALID:
                     line->setFlags(SHARED);
                     numMemTransac++;
                     return BUS_RD;
                     break;     
                  default: break;
               }
               break;
            case MSIPLUS:
               switch(line->getFlags())
               {
                  case MODIFIED:
                     return NOSIG;
                     break;  
                  case SHARED:
                     return NOSIG;
                     break;
                  case INVALID:
                     line->setFlags(SHARED);
                     numMemTransac++;
                     return BUS_RD;
                     break;     
                  default: break;
               }
               break;
            case MESI:
               switch(line->getFlags())
               {
                  case MODIFIED:  
                     return NOSIG;
                     break;
                  case EXCLUSIVE:
                     return NOSIG;
                     break;
                  case SHARED:
                     return NOSIG;
                     break;
                  case INVALID:
                     numMemTransac++;
                     return BUS_RD;
                     break;     
                  default: break;
               }
               break;
            case MESIFILTER:
               switch(line->getFlags())
               {
                  case MODIFIED:  
                     return NOSIG;
                     break;
                  case EXCLUSIVE:
                     return NOSIG;
                     break;
                  case SHARED:
                     return NOSIG;
                     break;
                  case INVALID:
                     //line->setFlags(SHARED);
                     numMemTransac++;
                     return BUS_RD;
                     break;     
                  default: break;
               }
               break;      
            default: break;
         }
      }
   }

   return NOSIG;
}


bool Cache::Snoop(uint32_t busTransac, ulong addr, ulong protocol)
{
   cacheLine * line = findLine(addr);
   if(protocol == MESIFILTER)
   {
      if(historyFilter->findLine(addr))
      {
         filterSnoop++;
         return false;
      }
   }

   if(line)
   {
      usefulSnoop++;
      switch(protocol)
      {
         case MSI:
            switch(line->getFlags())
            {
               case MODIFIED:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        line->setFlags(SHARED);
                        writeBack(addr);
                        numInter++;
                        flushes++;
                        break;
                     case BUS_RDX:
                        line->setFlags(INVALID);
                        writeBack(addr);
                        numInval++;
                        flushes++;
                        break;   
                     default: break;
                  }
                  break;
               case SHARED:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        break;
                     case BUS_RDX:
                        line->setFlags(INVALID);
                        numInval++;
                        break;   
                     default: break;
                  }
                  break;
               case INVALID:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        break;
                     case BUS_RDX:
                        break;   
                     default: break;
                  }
                  break;
               default: break;
            }
            break;
         case MSIPLUS:
            switch(line->getFlags())
            {
               case MODIFIED:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        line->setFlags(SHARED);
                        writeBack(addr);
                        numInter++;
                        flushes++;
                        break;
                     case BUS_RDX:
                        line->setFlags(INVALID);
                        writeBack(addr);
                        numInval++;
                        flushes++;
                        break; 
                     default: break;
                  }
                  break;
               case SHARED:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        break;
                     case BUS_RDX:
                        line->setFlags(INVALID);
                        numInval++;
                        break;  
                     case BUS_UPGR:
                        line->setFlags(INVALID);
                        numInval++;
                        break; 
                     default: break;
                  }
                  break;
               case INVALID:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        break;
                     case BUS_RDX:
                        break;  
                     case BUS_UPGR:
                        break; 
                     default: break;
                  }
                  break;
               default: break;
            }
            break;
         case MESI:
            switch(line->getFlags())
            {
               case MODIFIED:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        line->setFlags(SHARED);
                        writeBack(addr);
                        flushes++;
                        numInter++;
                        break;
                     case BUS_RDX:
                        line->setFlags(INVALID);
                        writeBack(addr);
                        flushes++;
                        numInval++;
                        break; 
                     default: break;
                  }
                  break;
               case EXCLUSIVE:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        line->setFlags(SHARED);
                        numInter++;
                        break;
                     case BUS_RDX:
                        line->setFlags(INVALID);
                        numInval++;
                        break;
                     default: break;
                  }
                  break;
               case SHARED:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        break;
                     case BUS_RDX:
                        line->setFlags(INVALID);
                        numInval++;
                        break;  
                     case BUS_UPGR:
                        line->setFlags(INVALID);
                        numInval++;
                        break; 
                     default: break;
                  }
                  break;
               case INVALID:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        break;
                     case BUS_RDX:
                        break;  
                     case BUS_UPGR:
                        break; 
                     default: break;
                  }
                  break;
               default: break;
            }
            break;
         
         case MESIFILTER:
            switch(line->getFlags())
            {
               case MODIFIED:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        line->setFlags(SHARED);
                        writeBack(addr);
                        flushes++;
                        numInter++;
                        break;
                     case BUS_RDX:
                        line->setFlags(INVALID);
                        historyFilter->fillLine(addr);
                        writeBack(addr);
                        flushes++;
                        numInval++;
                        break; 
                     default: break;
                  }
                  break;
               case EXCLUSIVE:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        line->setFlags(SHARED);
                        numInter++;
                        break;
                     case BUS_RDX:
                        line->setFlags(INVALID);
                        historyFilter->fillLine(addr);
                        numInval++;
                        break;
                     default: break;
                  }
                  break;
               case SHARED:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        break;
                     case BUS_RDX:
                        line->setFlags(INVALID);
                        historyFilter->fillLine(addr);
                        numInval++;
                        break;  
                     case BUS_UPGR:
                        line->setFlags(INVALID);
                        historyFilter->fillLine(addr);
                        numInval++;
                        break; 
                     default: break;
                  }
                  break;
               case INVALID:
                  switch(busTransac)
                  {
                     case BUS_RD:
                        break;
                     case BUS_RDX:
                        break;  
                     case BUS_UPGR:
                        break; 
                     default: break;
                  }
                  break;
               default: break;
            }
            break;

         default: break;
      }
      
      return true;
   }

   if (protocol == MESIFILTER) 
   {
      historyFilter->fillLine(addr);
   }
   wastedSnoop++;
   return false;
}

void Cache::secondRound(bool C, ulong addr, uint32_t transac)
{
   cacheLine * line = findLine(addr);
   if(transac == BUS_RD)
   {
      if(C){
      line->setFlags(SHARED);
      cache2cache++;
      numMemTransac--;
      }
      else{
         line->setFlags(EXCLUSIVE);
      }
   }

   if(transac == BUS_RDX)
   {
      if(C)
      {
         line->setFlags(MODIFIED);
         cache2cache++;
         numMemTransac--;
      }
   }
   
}

/*look up line*/
cacheLine * Cache::findLine(ulong addr)
{
   ulong i, j, tag, pos;
   
   pos = assoc;
   tag = calcTag(addr);
   i   = calcIndex(addr);
  
   for(j=0; j<assoc; j++)
   if(cache[i][j].isValid()) {
      if(cache[i][j].getTag() == tag)
      {
         pos = j; 
         break; 
      }
   }
   if(pos == assoc) {
      return NULL;
   }
   else {
      return &(cache[i][pos]); 
   }
}

/*upgrade LRU line to be MRU line*/
void Cache::updateLRU(cacheLine *line)
{
   line->setSeq(currentCycle);  
}

/*return an invalid line as LRU, if any, otherwise return LRU line*/
cacheLine * Cache::getLRU(ulong addr)
{
   ulong i, j, victim, min;

   victim = assoc;
   min    = currentCycle;
   i      = calcIndex(addr);
   
   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].isValid() == 0) { 
         return &(cache[i][j]); 
      }   
   }

   for(j=0;j<assoc;j++)
   {
      if(cache[i][j].getSeq() <= min) { 
         victim = j; 
         min = cache[i][j].getSeq();}
   } 

   assert(victim != assoc);
   
   return &(cache[i][victim]);
}

/*find a victim, move it to MRU position*/
cacheLine *Cache::findLineToReplace(ulong addr)
{
   cacheLine * victim = getLRU(addr);
   updateLRU(victim);
  
   return (victim);
}

/*allocate a new line*/
cacheLine *Cache::fillLine(ulong addr)
{ 
   ulong tag;
  
   cacheLine *victim = findLineToReplace(addr);
   assert(victim != 0);
   
   if(victim->getFlags() == MODIFIED) {
      writeBack(addr);
   }
      
   tag = calcTag(addr);   
   victim->setTag(tag);
   victim->setFlags(SHARED);    
   /**note that this cache line has been already 
      upgraded to MRU in the previous function (findLineToReplace)**/

   return victim;
}

void Cache::removeFromCache(ulong addr)
{
   cacheLine *victim = findLine(addr);
   victim->setFlags(INVALID);
   victim->setTag(0);

}

void Cache::printStats(uint32_t procNum, ulong protocol)
{ 
   printf("============ Simulation results (Cache %d) ============\n", procNum);
   /****print out the rest of statistics here.****/
   /****follow the ouput file format**************/
   printf("01. number of reads: %d\n", (int)reads);
   printf("02. number of read misses: %d\n", (int)readMisses);
   printf("03. number of writes: %d\n", (int)writes);
   printf("04. number of write misses: %d\n", (int)writeMisses);
   printf("05. total miss rate: %.2f%%\n", ((double)(readMisses+writeMisses))/((double)(reads+writes))*100);
   printf("06. number of writebacks: %d\n", (int)writeBacks);
   printf("07. number of cache-to-cache transfers: %d\n", (int)cache2cache);
   printf("08. number of memory transactions: %d\n", (int)numMemTransac);
   printf("09. number of interventions: %d\n", (int)numInter);
   printf("10. number of invalidations: %d\n", (int)numInval);
   printf("11. number of flushes: %d\n", (int)flushes);
   printf("12. number of BusRdX: %d\n", (int)numBusRdX);
   printf("13. number of BusUpgr: %d\n", (int)numBusUp);

   if(protocol == MESIFILTER)
   {

      printf("14. number of useful snoops: %d\n", (int)usefulSnoop);
      printf("15. number of wasted snoops: %d\n", (int)wastedSnoop);
      printf("16. number of filtered snoops: %d\n", (int)(filterSnoop));
      
   }
}

/*******************************************************
                          cache.h
********************************************************/

#ifndef CACHE_H
#define CACHE_H

#include <cmath>
#include <iostream>

typedef unsigned long ulong;
typedef unsigned char uchar;
typedef unsigned int uint;

/****add new states, based on the protocol****/
enum {
   MODIFIED = 0,
   EXCLUSIVE,
   SHARED,
   INVALID 
};


//Added enum for bus transactions that can be returned from access
enum {
   BUS_RD = 0,
   BUS_RDX,
   BUS_UPGR,
   NOSIG
};

//Added enum for protocol used
enum {
   MSI = 0,
   MSIPLUS,
   MESI,
   MESIFILTER
};

class cacheLine 
{
protected:
   ulong tag;
   ulong Flags;   // 0:invalid, 1:valid, 2:dirty 
   ulong seq; 
   
 
public:
   cacheLine()                { tag = 0; Flags = INVALID; }
   ulong getTag()             { return tag; }
   ulong getFlags()           { return Flags;}
   ulong getSeq()             { return seq; }
   void setSeq(ulong Seq)     { seq = Seq;}
   void setFlags(ulong flags) {  Flags = flags;}
   void setTag(ulong a)       { tag = a; }
   void invalidate()          { tag = 0; Flags = INVALID; } //useful function
   bool isValid()             { return ((Flags) != INVALID); }
   
   
};

class Cache
{
protected:
   Cache* historyFilter;
   ulong size, lineSize, assoc, sets, log2Sets, log2Blk, tagMask, numLines;
   ulong reads,readMisses,writes,writeMisses,writeBacks,flushes,cache2cache, numBusRdX, numBusUp, numMemTransac, numInter, numInval;
   ulong usefulSnoop, wastedSnoop, filterSnoop;
   
   //******///
   //add coherence counters here///
   //******///

   cacheLine **cache;
   ulong calcTag(ulong addr)     { return (addr >> (log2Blk) );}
   ulong calcIndex(ulong addr)   { return ((addr >> log2Blk) & tagMask);}
   ulong calcAddr4Tag(ulong tag) { return (tag << (log2Blk));}
   
public:
    ulong currentCycle;  
     
    Cache(int,int,int, ulong);
   ~Cache() { delete cache;}
   
   cacheLine *findLineToReplace(ulong addr);
   cacheLine *fillLine(ulong addr);
   cacheLine * findLine(ulong addr);
   cacheLine * getLRU(ulong);
   void removeFromCache(ulong);

   ulong getRM()     {return readMisses;} 
   ulong getWM()     {return writeMisses;} 
   ulong getReads()  {return reads;}       
   ulong getWrites() {return writes;}
   ulong getWB()     {return writeBacks;}
   
   void writeBack(ulong) {writeBacks++; numMemTransac++;}
   uint32_t Access(ulong,uchar,ulong);
   void printStats(uint32_t, ulong);
   void updateLRU(cacheLine *);
   bool Snoop(uint32_t, ulong, ulong); 
   void secondRound(bool, ulong, uint32_t);

   
   

   //******///
   //add other functions to handle bus transactions///
   //******///

};

#endif

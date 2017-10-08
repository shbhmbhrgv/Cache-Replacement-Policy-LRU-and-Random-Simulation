///////========================================================================//////
/* 
 * File Name : cache.cpp
 * Created By: Shubham Bhargava
 * File Description: Cache Simulator program, implementing LRU and Random
 */
///////========================================================================//////

#include "stdio.h"
#include "math.h"
#include <iostream>
#include <sstream>
#include <cstdint>
#include <string>
#include <algorithm>
#include <vector>
#include <list>
#include <fstream>
#include <unordered_map>

using std::string;
using std::vector;
using std::list;

#define DEBUG 0

typedef unsigned long long int uintull;

//Cache Address Line
struct CacheLine{
    uintull tag;
    uintull index;
    uintull offset;
};

//Class of a Block in set
class Block {
 
public:

    Block(unsigned long blkTag):
        valid(false), tag(blkTag)
    {
    }

    ~Block()
    {
    }

    bool valid;
    unsigned long tag;
};

//Class of a CacheSet
class Set {

public:

    Set(int nBlocks, int blkSize) : numBlocks(nBlocks), blockSize(blkSize),
                                    blocks(nBlocks, Block(0))
    {
    }

    ~Set()
    {
    }

    int numBlocks;
    int blockSize;
    vector<Block> blocks;   //vector of n blocks, where n is n-associativity
     
    std::unordered_map<int, int> blockIdxVsUsageMap;    //maintains map of block index vs its position in used block vector
    std::list<int> usedBlocksVec; //vector of used blocks, first index gives the least recently used block index, and last is most recently used

};

//Class of Cache
class Cache {

public:
    //Constructor
    Cache(char repl, int cSize, int bSize, int sSize, 
            int assoc, int nSets, CacheLine cacheLine):
            replPolicy(repl), cacheSize(cSize), blockSize(bSize),
            setSize(sSize), setAssoc(assoc), numSets(nSets),
            readAccessCnt(0), readHitCnt(0), writeAccessCnt(0),
            writeHitCnt(0), sets(numSets, Set(assoc, bSize)),
            cacheAddrLine(cacheLine)
    {
    }

    //Destructor
    ~Cache()
    {
    }

    char replPolicy;

    //All sizes are in bytes except cachesize
    int cacheSize; // in KB  
    int blockSize;  
    int setSize;

    int setAssoc;
    int numSets;

    uintull readAccessCnt;
    uintull readHitCnt;
    uintull writeAccessCnt;
    uintull writeHitCnt; 
     
    
    vector<Set> sets;   //vector of sets, index vs set
    CacheLine cacheAddrLine;
  
};

//API to extract out the offset, index, and tag parameters
uintull createMask(uintull retval, uintull current, uintull length, uintull offset) 
{
	if(current == length)
		return ( retval - 1 ) << offset;
	else return createMask(retval << 1, current + 1, length, offset);
} 

uintull createMask(uintull length, uintull offset) 
{
	return createMask(1, 0, length, offset);
}

//Divide the address into tag, index, and offset
//addrDivision structure contents are filled
void getInstructionDetails(const CacheLine& cacheDiv, uintull addr, CacheLine& addrDivision) 
{
	uintull offset = 64 - cacheDiv.tag;
	addrDivision.tag = ( addr & createMask(cacheDiv.tag, offset) ) >> offset;
	addrDivision.index = ( addr & createMask(cacheDiv.index, cacheDiv.offset) ) >> cacheDiv.offset;
	addrDivision.offset = ( addr & createMask(cacheDiv.offset, 0) );
}

//Given a string from file, get its read or write value and its address in hexadecimal
bool getInstruction(const string& str, char& readOrWrite, unsigned long long int& addr)
{
    if(str.empty()){
        std::cout << " Empty string. \n";
        return false;
    }

    if(str.find("r") != string::npos){
        readOrWrite = 'r';
    }else if(str.find("w") != string::npos){
        readOrWrite = 'w';
    }else{
        return false;
    }

    string addrStr = str.substr(2);
    std::istringstream iss(addrStr);
    iss >> std::hex >> addr;
    return true;
}

//check if address is a valid one, updates containers of list and hashmap for LRU
bool isAddrValid(Cache& cacheObj, const CacheLine& addrLine)
{
    int assoc = cacheObj.setAssoc;

    for(int i = 0; i < assoc; ++i){

        if(((cacheObj.sets[addrLine.index]).blocks[i].valid) &&
            (cacheObj.sets[addrLine.index].blocks[i].tag == addrLine.tag)){
        
               
            if(cacheObj.setAssoc == 'l'){
                //first find the index in the vector
                //then delete from the vector
                //push back into the vector
                auto itr = cacheObj.sets[addrLine.index].blockIdxVsUsageMap.find(i);
                if(itr != (cacheObj.sets[addrLine.index].blockIdxVsUsageMap).end()){
                    int vecIdx = itr->second;
                    
                    auto listItr = std::find((cacheObj.sets[addrLine.index].usedBlocksVec).begin(),
                         (cacheObj.sets[addrLine.index].usedBlocksVec).end(),i);
                    if(listItr != (cacheObj.sets[addrLine.index].usedBlocksVec).end()){
                        (cacheObj.sets[addrLine.index].usedBlocksVec).erase(listItr);
                    }
                    //erase from vector
                    //(cacheObj.sets[addrLine.index].usedBlocksVec).erase
                      //  (cacheObj.sets[addrLine.index].usedBlocksVec.begin() + vecIdx);
                    
                    //erase from map
                    cacheObj.sets[addrLine.index].blockIdxVsUsageMap.erase(itr);

                    //insert into vector and get new key
                    vecIdx = (cacheObj.sets[addrLine.index].usedBlocksVec).size();
                    cacheObj.sets[addrLine.index].usedBlocksVec.push_back(i);

                    //insert into map with new key
                    cacheObj.sets[addrLine.index].blockIdxVsUsageMap.insert
                        (std::make_pair(i, vecIdx));

                    
                }
            }

            return true;
        
        
        }
        

    }

    return false;
}

//inserts the corresponding addr into cache, depending upon whether LRU or random
void insertIntoCache(Cache& cacheObj, const CacheLine& reqAddr) 
{

    int assoc = cacheObj.setAssoc;
    
    //first check if any of the block's tag is invalid
    for(int i = 0; i < assoc; ++i) {
		if(cacheObj.sets[reqAddr.index].blocks[i].valid == false) {
            //mark tag to true and assign it
			cacheObj.sets[reqAddr.index].blocks[i].valid = true;
			cacheObj.sets[reqAddr.index].blocks[i].tag = reqAddr.tag;

            //insert into vector and hash map
            int vecIdx = (cacheObj.sets[reqAddr.index].usedBlocksVec).size();

            cacheObj.sets[reqAddr.index].usedBlocksVec.push_back(i);


            cacheObj.sets[reqAddr.index].blockIdxVsUsageMap.insert
                    (std::make_pair(i, vecIdx));

			return;
		}
	}
if(DEBUG)
{
    std::cout << "\n\n";
    for(auto i: (cacheObj.sets[reqAddr.index].usedBlocksVec)){
        std::cout << " elem: "<< i << "\n";
    }
}
    //now use random or lru depending upon the replacement policy
    char repl_Policy = cacheObj.replPolicy;

    if(repl_Policy == 'l'){//delete from the list the top element, and from hashmap
        //corresponding entry, and reinsert the new element at the back of list and hashmap

        //pick the top element of block vector in set, which gives the block index
        int idx = (cacheObj.sets[reqAddr.index].usedBlocksVec).front();
        (cacheObj.sets[reqAddr.index].usedBlocksVec).erase
            ((cacheObj.sets[reqAddr.index].usedBlocksVec).begin());

        //std::cout << "Index of block: "<<idx << "\n";
        //std::cout << " index after deletion: " << (cacheObj.sets[reqAddr.index].usedBlocksVec)[0] << "\n";
        auto itr = (cacheObj.sets[reqAddr.index].blockIdxVsUsageMap).find(idx);
        if(itr != (cacheObj.sets[reqAddr.index].blockIdxVsUsageMap).end()){
            (cacheObj.sets[reqAddr.index].blockIdxVsUsageMap).erase(itr);
            
            int vecIdx = (cacheObj.sets[reqAddr.index].usedBlocksVec).size();

            //std::cout << "Vector idx: "<< vecIdx << "\n";
            cacheObj.sets[reqAddr.index].usedBlocksVec.push_back(idx);


            cacheObj.sets[reqAddr.index].blockIdxVsUsageMap.insert
                    (std::make_pair(idx, vecIdx));
            cacheObj.sets[reqAddr.index].blocks[idx].tag = reqAddr.tag;
        }

                
    }else {
        srand(time(NULL));
        if(assoc > 1)
            --assoc;
        int replIdx = rand() % assoc;
        cacheObj.sets[reqAddr.index].blocks[replIdx].tag = reqAddr.tag;
    }
	
}




int main(int argc, char** argv) {

    int cache_Cap = 0;
    int cache_Assoc = 0;
    int cache_BlockSize = 0;
    char cache_Repl_Policy = 'l';   //default assigning to l(LRU)


    if ( argc != 5 ) { //number of args should be 4
        std::cout << "Number of arguments entered: " << argc << 
            " Invalid number of arguments, Please input the correct number of arguments and start again\n"; 
        return 1;
    } else  {
        
        //Assume the 4 arguments in the following order:
        /*1. nk: the capacity of the cache in kilobytes (an int)
          2. assoc: the associativity of the cache (an int)
          3. blocksize: the size of a single cache block in bytes (an int)
          4. repl: the replacement policy (a char); 'l' means LRU, 'r' means random*/

        cache_Cap = std::stoi(argv[1]);
        
        cache_Assoc = std::stoi(argv[2]);
        
        cache_BlockSize = std::stoi(argv[3]);
        
        cache_Repl_Policy = *argv[4];
        
if(DEBUG){
        std::cout << "Cache size: "<< cache_Cap << " associativity: " << cache_Assoc <<
             " block size: " << cache_BlockSize << " repl: " << cache_Repl_Policy << "\n";
}
        int setSize = cache_Assoc * cache_BlockSize;
        int numSets = (cache_Cap * 1024)/setSize; 

        uintull setBits = log2(numSets);
        uintull blockOffsetBits = log2(cache_BlockSize);
        uintull tagBits = 64 - setBits - blockOffsetBits;

        CacheLine cacheLine;
        cacheLine.tag = tagBits;
        cacheLine.offset = blockOffsetBits;
        cacheLine.index = setBits;

        //Initialise Cache object, and set all the default bits to be false
        Cache cacheObj = Cache(cache_Repl_Policy, cache_Cap, cache_BlockSize, setSize,
                                                    cache_Assoc, numSets, cacheLine); 
       
if(DEBUG){
        std::cout << "cache cap: "<< cache_Cap << " block size: "<< cache_BlockSize <<
            " set size: " << setSize << " assoc: "<< cache_Assoc << 
            " num sets " << numSets << " tagBits: "<< tagBits << 
            " offset: "<< blockOffsetBits << " index: "<< setBits << "\n";
}

        
        string fileListName = "files.txt";
        std::ifstream fileListStream(fileListName);
        if(!fileListStream){
            std::cout << "Failed to open file: " << fileListName << "\n";
            return 1;
        }

        //std::string fileName;
        //while(std::getline(fileListStream, fileName)){

            
            //Read the file line by line and process it
            std::string str;
            int lineNo = 0; //keep line number for debugging purposes
            
            
            while (/*std::cin >> str*/std::getline(std::cin, str)) {
                ++lineNo;
            if(DEBUG)
            {
                //std::cout << " Line: " << lineNo << " String: " << str << std::endl;
            }   
                //Process as a read/write instruction, and take its address
                char readOrWrite;
                unsigned long long int addr = 0;

                //extract address and read or write type from the instruction string
                if(!getInstruction(str, readOrWrite, addr)){
                    std::cout << " Failed to get instruction at line no: " << lineNo << 
                                " for string: " << str << "\n"; 
                    return 1;
                }

                CacheLine addrDivision;
                getInstructionDetails(cacheLine, addr, addrDivision);

                //Access this instruction in cache and take necessary decision,
                //if found then alright, else if found and tag invalid, update,
                //and increase the appropriate count
                //if not found, fill using the replacement policy
                //
                //
                if(readOrWrite == 'r'){
                    (cacheObj.readAccessCnt)++;
                }else if (readOrWrite == 'w'){
                    (cacheObj.writeAccessCnt)++;
                }

            if(DEBUG)
            {
                //std::cout << " addr offset: "<< addrDivision.offset << 
                  //          " tag: "<< addrDivision.tag << 
                    //        " index: "<< addrDivision.index << "\n";
            }
                if(isAddrValid(cacheObj, addrDivision)){
                    if(readOrWrite == 'r'){
                        (cacheObj.readHitCnt)++;
                    }else if(readOrWrite == 'w'){
                        (cacheObj.writeHitCnt)++;
                    }
                }else{
                    insertIntoCache(cacheObj, addrDivision);
                }

            }

        //}

        uintull readMiss = cacheObj.readAccessCnt - cacheObj.readHitCnt;
        uintull writeMiss = cacheObj.writeAccessCnt - cacheObj.writeHitCnt;
        double readMissPerc = (double)(readMiss)/(double)(cacheObj.readAccessCnt) * 100;
        double writeMissPerc = (double)(writeMiss)/(double)(cacheObj.writeAccessCnt) * 100;
        uintull totalMiss = readMiss + writeMiss;
        double totalMissPerc = (double)(totalMiss)/(double)(cacheObj.readAccessCnt +
                                                            cacheObj.writeAccessCnt) * 100;


        std::cout << totalMiss << " " << totalMissPerc << " " 
                << readMiss << " " << readMissPerc << " "
                << writeMiss << " " << writeMissPerc << "\n";
                    

        /*std::cout << "total read miss: "<< cacheObj.readAccessCnt - cacheObj.readHitCnt << 
                " total write miss: "<< cacheObj.writeAccessCnt - cacheObj.writeHitCnt << 
                " read percentage: " << double(cacheObj.readHitCnt)/double(cacheObj.readAccessCnt) <<
                " write percentage: " << cacheObj.writeHitCnt/cacheObj.writeAccessCnt << "\n";
        */

    }
}

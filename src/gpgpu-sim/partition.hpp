#ifndef PARTITION_HPP
#define PARTITION_HPP
//code here

/*author sjq
**email ivy22233qiu@live.com
**create time:2018-06-10
**introduction:algorithm for l2 dynamic partition
*/
//#include "../option_parser.h"
#include <vector>
#include <list>
#define ULL unsigned long long

#define L(x) std::list<x >

#define V(x) std::vector<x >

//#include "gpu-sim.h"
//#include "gpu-cache.h"
typedef unsigned long long new_addr_type;
typedef unsigned long long cycle_size;

struct partition_config
{
    partition_config();
    void init(int assoc,int set,int mem_partiton=6,int sub_partition_per_mem=2);
    //void reg_options( OptionParser * opp);

    //members
    int n_assoc;
    int n_set;
    bool enable_partition_unit;
    int app_num;
    cycle_size activeCycles;
    int samplingWidth;

    int reSetPolicy;//0=devide by 2,1=set to 0

};
class partition_unit{
    //friend std::ostream& operator <<(std::ostream & out,partition_unit& unit);
    public: 
    partition_unit(const partition_config&);
    void access(unsigned core_id,unsigned set_idx,new_addr_type tagId);
    void res_fail(unsigned core_id,unsigned set_idx);
    const std::vector<int>& getBestPartition() const;
    void setBestPartition();
    void reSet();
    void save();
    void printStat();
    std::vector<int> currPartition;
    const partition_config& get_config() const{
        return m_config;
    }
    std::vector<unsigned> get_best_local();
    void print_miss_stat();
    void print_stack_compete(unsigned setIdx);




    private:
    //unsigned long long total_access;
    const partition_config& m_config;
    unsigned num_stack;
    V(ULL) appAccess;//2
    
    V(V(ULL)) sampleing_access;//sampling num* appNum
    V(int) best_partition;
    
    V(V(L(ULL))) l2_sim_stack_array;//8*2*16
    V(V(ULL)) counter;//overall counter2*16//
    V(V(V(ULL))) local_counter;//local counter8*2*16
    
    
    V(V(unsigned)) partition_stat;
    V(L(ULL)) compete_stack;
    V(V(ULL)) compete_hit_conter;//8*2
};
//std::ostream& operator <<(std::ostream & out,partition_unit& unit){
//
//}


#endif
#include"partition.hpp"
#include<cassert>
#include<fstream>

#ifdef SJQ_DEBUG
#include<iostream>
template<typename T> std::ostream& operator<<(std::ostream& out,std::list<T> m_list){
    typename std::list<T>::iterator it=m_list.begin();
    int i=0;
    while(it!=m_list.end()){
        out<<"["<<i++<<"]"<<*it<<" --> ";
        it++;
    }
    out<<"END"<<std::endl;
    return out;
}
#endif

partition_unit::partition_unit(const partition_config& config):
    m_config(config),
    num_stack(m_config.n_set/m_config.samplingWidth),
    appAccess(config.app_num,0),
    sampleing_access(num_stack,\
                    V(ULL)(config.app_num,0)),
    best_partition(m_config.app_num,m_config.n_assoc/2),
    
    l2_sim_stack_array(num_stack,V(L(ULL))(m_config.app_num,L(ULL)(16,-1))),
    counter(m_config.app_num,\
            V(ULL)(m_config.n_assoc,0)),
    local_counter(num_stack,\
                V(V(ULL))(m_config.app_num,V(ULL)(16,0))),
    compete_stack(num_stack,L(ULL)(m_config.n_assoc,-1)),
    compete_hit_conter(num_stack,V(ULL)(m_config.app_num,0))
      
{
    
    if(m_config.samplingWidth==0){
        abort();
        return;
    }
}
partition_config::partition_config(){}



void partition_unit::access(unsigned core_id,unsigned set_idx,new_addr_type tagId){
    
    
    
    if(set_idx%m_config.samplingWidth==0){
        
        unsigned appId=core_id/15;//TODO
        unsigned stackId=set_idx/m_config.samplingWidth;
        appAccess[appId]++;
        sampleing_access[stackId][appId]++;
        assert(stackId<num_stack);
        

        
        std::list<new_addr_type>::iterator it=l2_sim_stack_array[stackId][appId].begin();
        L(ULL)::iterator it_compete=compete_stack[stackId].begin();
        int numToAdd=0;
        for(int i=0;i<m_config.n_assoc;i++){
            assert(it!=l2_sim_stack_array[stackId][appId].end());
            //start partition conter
            if(numToAdd==0&&*it==-1){//miss and have ivalid entry
                l2_sim_stack_array[stackId][appId].erase(it);
                l2_sim_stack_array[stackId][appId].push_front(tagId);
                break;
            }
            if(numToAdd==0&&*it==tagId){//hit
                
                numToAdd=1;
                l2_sim_stack_array[stackId][appId].push_front(tagId);
                it--;
                std::list<new_addr_type>::iterator newit=it;
                it++;
                l2_sim_stack_array[stackId][appId].erase(it);
                it=newit;
                
            }

            if(numToAdd==1){//hit
                local_counter[stackId][appId][i]++;
                counter[appId][i]++;
            }
            it++;

            

        }
        if(numToAdd==0&&it==l2_sim_stack_array[stackId][appId].end()){//miss and need to evict
            l2_sim_stack_array[stackId][appId].push_front(tagId);
            --it;
            l2_sim_stack_array[stackId][appId].erase(it);
            
            

        }
        for(int i=0;i<m_config.n_assoc;i++){
        //start competetion counter
            if(*it_compete==-1){//miss and have ivalid entry
                compete_stack[stackId].erase(it_compete);
                compete_stack[stackId].push_front(tagId);
                
                break;
            }
            if(*it_compete==tagId){//hit
                
                
                compete_stack[stackId].push_front(tagId);
                it_compete--;
                std::list<new_addr_type>::iterator newit_compete=it_compete;
                it_compete++;
                compete_stack[stackId].erase(it_compete);
                compete_hit_conter[stackId][appId]++;
                
                break;
                
            }
            it_compete++;
        }
        if(it_compete==compete_stack[stackId].end()){
            compete_stack[stackId].push_front(tagId);
            it_compete--;
            compete_stack[stackId].erase(it_compete);
        }
        #ifdef SJQ_DEBUG
         std::cout<<l2_sim_stack_array[stackId][appId]<<std::endl;
        #endif
    }
}


void partition_unit::setBestPartition() {
    std::vector<int> retVal(2,0);

    int max=0;
    for(int i=0;i<m_config.n_assoc-1;i++){//i=0~14
        if(counter[0][i]+counter[1][m_config.n_assoc-i-2]>max){
            max=counter[0][i]+counter[1][m_config.n_assoc-i-2];
            retVal[0]=i+1;
            retVal[1]=m_config.n_assoc-i-1;

        }
    }
    if(max==0){
        retVal[0]=m_config.n_assoc/2;
        retVal[1]=retVal[0];
    }
    best_partition=retVal;

}
const std::vector<int>& partition_unit::getBestPartition() const{
    return best_partition;
}
void partition_unit::reSet(){
    switch(m_config.reSetPolicy){
        case 0:
        for(int i=0;i<m_config.app_num;i++){
            for(int j=0;j<m_config.n_assoc;j++){
                counter[i][j]/=2;
            }
        }
        for(int i=0;i<m_config.n_set/m_config.samplingWidth;i++){
        
            for(int j=0;j<m_config.app_num;j++){
            
                for(int k=0;k<m_config.n_assoc;k++){
                    local_counter[i][j][k]/=2;
                }
            }
        }
        break;
        case 1:
        for(int i=0;i<m_config.app_num;i++){
            for(int j=0;j<m_config.n_assoc;j++){
                counter[i][j]=0;
            }
        }
        for(int i=0;i<m_config.n_set/m_config.samplingWidth;i++){    
            for(int j=0;j<m_config.app_num;j++){
                for(int k=0;k<m_config.n_assoc;k++){
                    local_counter[i][j][k]=0;
                }
            }
        }
        break;
        
    }
    partition_stat.push_back(get_best_local());
    print_miss_stat();

    
}
void partition_unit::printStat(){
    std::ofstream out("partiton_stat.txt");
    if(!out.is_open()){
        abort();
    }
    for(int i=0;i<partition_stat.size();i++){
        for(int j=0;j<partition_stat[i].size();j++){
            out<<partition_stat[i][j]<<" ";
        }
        out<<std::endl;
    }
    out.close();
}
std::vector<unsigned> partition_unit::get_best_local(){
    std::vector<unsigned> best_local;
    int max=0;
    for(int i=0;i<m_config.n_set/m_config.samplingWidth;i++){
        best_local.push_back(8);
        for(int j=0;j<m_config.n_assoc-1;j++){//i=0~14
            if(local_counter[i][0][j]+local_counter[i][1][m_config.n_assoc-j-2]>max){
                max=local_counter[i][0][j]+local_counter[i][1][m_config.n_assoc-j-2];
                best_local[i]=j+1;
                

            }
        }
        if(max==0){
            best_local[i]=m_config.n_assoc/2;
        }
    }
    return best_local;
}
void partition_unit::print_miss_stat(){
    static ULL m_cycle=0;
    std::ofstream out("miss_stat.txt",std::ios::app|std::ios::out);
    if(!out.is_open())
    {

        abort();
    }
    out<<m_cycle*m_config.activeCycles<<" ";
    for(int i=0;i<num_stack;i++){
        for(int j=0;j<m_config.app_num;j++){
            out<<sampleing_access[i][j]<<" "\
            <<compete_hit_conter[i][j]<< " ";
            
        }
    }
    out<<std::endl;
    out.close();
}
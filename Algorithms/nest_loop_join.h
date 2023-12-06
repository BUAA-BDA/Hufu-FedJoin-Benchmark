#pragma once
#include "utils.h"
#include <vector>

using namespace std;

class NestLoopJoin {
public:
    vector<Tuple> relation_;
    vector<SharedTuple> res_;
    vector<Tuple> res_val_;
    NestLoopJoin(Party* party, vector<Tuple>& relation, int size_s, int size_c, int size_res): 
        party_(party), relation_(relation), size_s_(size_s), size_c_(size_c), size_res_(size_res) {
        while (relation_.size() < max(size_c_, size_s_)) {
            relation_.push_back(*(new Tuple(0, 0)));
        }
    }
    void join();
    void join_simd();
    void theta_join_simd();
    void join_oram();
private:
    Party* party_;
    Circuit* circ_;
    int size_s_;
    int size_c_;
    int size_res_;
    const uint32_t ZERO = 0;
    const uint32_t ONE = 1;
    uint32_t* res_k_;
    uint32_t* res_d1_;
    uint32_t* res_d2_;
    uint32_t* res_index_;
    uint32_t cur_loc_ = 0;
    void write(share* l, share* k, share* d1, share* d2);
};
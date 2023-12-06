#pragma once
#include "utils.h"
#include <vector>

class SortMergeJoin {
public:
    vector<Tuple> relation_;
    vector<SharedTuple> res_;
    vector<Tuple> res_val_;
    bool is_pk_;
    SortMergeJoin(Party* party, vector<Tuple>& relation, int size_s, int size_c, int size_res):
        party_(party), relation_(relation), size_s_(size_s), size_c_(size_c), size_res_(size_res) {
        uint32_t pre = 0;
        if (size_res_ == 0) {
            cout << "please set res_size!" << endl;
            exit(0);
        }
        sort(relation_.begin(), relation_.end());
        op_cnt_ = (size_res_ + size_c + size_s) * 40;
        while (relation_.size() < max(size_c_, size_s_)) {
            relation_.push_back(*(new Tuple(0, 0)));
        }
        for (auto r : relation_) {
            if (party_->get_role() == SERVER) {
                ind_vec_.push_back(r.k == pre? 1 : 0);
            }
            else {
                ind_vec_.push_back(r.k != pre? 1 : 0);
            }
            pre = r.k;
        }
    }
    void join();
    void join_PKFK();
    void theta_join();
    
private:
    Party* party_;
    Circuit* circ_;
    int size_s_;
    int size_c_;
    int size_res_;
    int op_cnt_;
    vector<uint32_t> ind_vec_;
    const uint32_t ZERO = 0;
    const uint32_t ONE = 1;
    const uint32_t NIL = -1;
};
#pragma once
#include "utils.h"
#include <set>
#include <vector>

using namespace std;

class HashJoin {
public:
    vector<Tuple> relation_;
    vector<SharedTuple> res_;
    vector<Tuple> res_val_;
    HashJoin(Party* party, vector<Tuple>& relation, uint32_t size_s, uint32_t size_c, uint32_t key_range): 
        party_(party), relation_(relation), size_s_(size_s), size_c_(size_c), table_size_(key_range) {
        if (key_range == 0) {
            cout << "please set key_range!" << endl;
            exit(0);
        }
        hash_table_key_ = vector<uint32_t>(table_size_, NIL);
        hash_table_data_ = vector<uint32_t>(table_size_, NIL);
        if (party_->get_role() == SERVER) {
            set<uint32_t> keys;
            for (auto r : relation) {
                keys.insert(r.k);
            }
            if (relation.size() != keys.size()) {
                cout << "HashJoin not suit for FK-FK join!" << endl;
                exit(0);
            }
            construct_hash_table();
        }
        while (relation_.size() < max(size_c_, size_s_)) {
            relation_.push_back(*(new Tuple(0, 0)));
        }
    }
    void join();
    
private:
    Party* party_;
    Circuit* circ_;
    uint32_t size_s_;
    uint32_t size_c_;
    uint32_t table_size_;
    vector<uint32_t> hash_table_key_;
    vector<uint32_t> hash_table_data_;
    const uint32_t NIL = -1;
    const uint32_t ZERO = 0;

    uint32_t equal_hash(uint32_t k) {
        return k;
    }
    void construct_hash_table();
    void oram_linear(share* s_select, share** ret_k, share** ret_d);
};
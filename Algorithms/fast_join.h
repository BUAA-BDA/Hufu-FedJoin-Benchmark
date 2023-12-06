#pragma once
#include "utils.h"
#include <set>
#include <vector>

using namespace std;

class FastJoin {
public:
    vector<Tuple> relation_;
    vector<SharedTuple> res_;
    vector<Tuple> res_val_;
    FastJoin(Party* party, vector<Tuple>& relation, uint32_t size_s, uint32_t size_c): 
        party_(party), relation_(relation), size_s_(size_s), size_c_(size_c) {
        bin_num_ = size_s_ * 2 + 11;
        cuckoo_key_ = vector<uint32_t>(bin_num_);
        cuckoo_data_ = vector<uint32_t>(bin_num_);
        if (party_->get_role() == SERVER) {
            set<uint32_t> keys;
            for (auto r : relation) {
                if (keys.count(r.k)) {
                    cout << "Duplication of " << r.k << endl;
                }
                keys.insert(r.k);
            }
            if (relation.size() != keys.size()) {
                cout << "FastJoin not suit for FK-FK join!" << endl;
                exit(0);
            }
            construct_cuckoo_hash_table();
        }
        else {
            construct_permute_table();
        }
        for (int i = 0; i < size_c_; i++) {
            if (party_->get_role() == CLIENT) {
                client_k_.push_back(relation_[i].k);
                client_d_.push_back(relation_[i].d);
            }
            else {
                client_k_.push_back(0);
                client_d_.push_back(0);
            }
        }
    }
    void join();
    
private:
    Party* party_;
    Circuit* circ_;
    uint32_t size_s_;
    uint32_t size_c_;
    uint32_t bin_num_;
    vector<uint32_t> cuckoo_key_;
    vector<uint32_t> cuckoo_data_;
    vector<uint32_t> permute_table0_;
    vector<uint32_t> permute_table1_;

    vector<uint32_t> client_k_;
    vector<uint32_t> client_d_;
    vector<uint32_t> server_k0_;
    vector<uint32_t> server_d0_;
    vector<uint32_t> server_k1_;
    vector<uint32_t> server_d1_;
    
    const uint32_t NIL = -1;
    const uint32_t ZERO = 0;

    uint32_t single_hash(int key, int v) {
        unsigned int ret = 0;
        auto SEED = 32;
        MurmurHash3_x86_32(&key, 4, SEED + v, &ret);
        return ret;
    }

    void construct_cuckoo_hash_table() {
        for (int i = 0; i < size_s_; i++) {
            cuckoo_hash(relation_[i].k, relation_[i].d);
        }
    }

    void construct_permute_table() {
        for (int i = 0; i < size_c_; i++) {
            vector<uint32_t> index = {single_hash(relation_[i].k, 0) % bin_num_, 
                single_hash(relation_[i].k, 1) % bin_num_};
            permute_table0_.push_back(index[0]);
            permute_table1_.push_back(index[1]);
        }
    }

    void cuckoo_hash(uint32_t key, uint32_t data);
    
    void obliv_switch();
};
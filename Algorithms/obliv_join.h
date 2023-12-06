#pragma once
#include "utils.h"
#include "bitonic_sort.h"
#include <vector>

using namespace std;

class OblivJoin {
public:
    vector<Tuple> relation_;
    OblivJoin(Party* party, vector<Tuple>& relation, uint32_t size_s, uint32_t size_c, int size_dummy): 
        party_(party), relation_(relation), size_s_(size_s), size_c_(size_c), size_dummy_(size_dummy) {
        size_aug_ = size_c + size_s;
        sort(relation_.begin(), relation_.end());
        while (relation_.size() < max(size_c_, size_s_)) {
            relation_.push_back(*(new Tuple(0, 0)));
        }
    }
    void join();
    void join_PKFK();
private:
    Party* party_;
    Circuit* circ_;
    uint32_t size_s_;
    uint32_t size_c_;
    uint32_t size_res_;
    uint32_t size_aug_;
    vector<SharedTuple> aug_table_;
    // tid: inds[0]  alpha1: inds[1]  alpha2: inds[2]

    vector<Tuple> exp_table1_val_;
    vector<Tuple> exp_table2_val_;
    vector<SharedTuple> exp_table1_;
    vector<SharedTuple> exp_table2_;
    // alpha: inds[0]  other: inds[1]  f: inds[2]

    vector<SharedTuple> align_table_;
    vector<Tuple> align_table1_val_;
    vector<Tuple> align_table2_val_;
    // ii: inds[0]  div: inds[1]  mod: inds[2]

    vector<Tuple> res_table_;
    const uint32_t NIL = 0;
    const uint32_t ZERO = 0;
    const uint32_t ONE = 1;
    const uint32_t TWO = 2;
    int size_dummy_ = 0;
    void prepare_aug_table();
    void downward_scan();
    void upward_scan();
    void split_table();
    share* prepare_exp_table(vector<Tuple>& src, vector<SharedTuple>& dst);
    void obliv_expand(bool pkfk);
    void distribute(vector<SharedTuple>& table);
    void fill_blank(vector<SharedTuple>& table, share* s_limit);
    void prepare_align_table();
    void align();
};
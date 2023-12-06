#include "nest_loop_join.h"

void NestLoopJoin::join() {
    cout << "===== Nest Loop Join =====" << endl;
    party_->reset();
    res_.clear();
    circ_ = party_->get_circuit(S_BOOL);
    int cnt = 1;
    for (int i1 = 0; i1 < size_s_; i1++) {
        auto s_k1 = circ_->PutINGate(relation_[i1].k, 32, SERVER);
        auto s_d1 = circ_->PutINGate(relation_[i1].d, 32, SERVER);
        auto s_zero = circ_->PutCONSGate(ZERO, 32);
        auto s_k1_out = circ_->PutOUTGate(s_k1, ALL);
        for (int i2 = 0; i2 < size_c_; i2++) {
            auto s_k2 = circ_->PutINGate(relation_[i2].k, 32, CLIENT);
            auto s_d2 = circ_->PutINGate(relation_[i2].d, 32, CLIENT);
            auto s_eq = circ_->PutEQGate(s_k1, s_k2);
            auto s_out_k = circ_->PutMUXGate(s_k1, s_zero, s_eq);
            auto s_out_d1 = circ_->PutMUXGate(s_d1, s_zero, s_eq);
            auto s_out_d2 = circ_->PutMUXGate(s_d2, s_zero, s_eq);

            if (SHARED) {
                s_eq = circ_->PutSharedOUTGate(s_eq);
                s_out_k = circ_->PutSharedOUTGate(s_out_k);
                s_out_d1 = circ_->PutSharedOUTGate(s_out_d1);
                s_out_d2 = circ_->PutSharedOUTGate(s_out_d2);
            }

            else {
                s_eq = circ_->PutOUTGate(s_eq, ALL);
                s_out_k = circ_->PutOUTGate(s_out_k, ALL);
                s_out_d1 = circ_->PutOUTGate(s_out_d1, ALL);
                s_out_d2 = circ_->PutOUTGate(s_out_d2, ALL);
            }
            
            vector<share*> s_inds = {s_out_d2, s_eq};
            res_.push_back({s_out_k, s_out_d1, s_inds});
        }
        party_->exec();
        for (int i = 0; i < res_.size(); i++) {
            auto k = res_[i].k->get_clear_value<uint32_t>();
            auto d1 = res_[i].d->get_clear_value<uint32_t>();
            auto d2 = res_[i].inds[0]->get_clear_value<uint32_t>();
            auto ind = res_[i].inds[1]->get_clear_value<uint32_t>();
            if (ind) {
                cout << "(" << cnt << ") " << k << " " << d1 << " " << d2 << endl;
                cnt++;
            }
            // res_val_.push_back({k, d1, d2, {ind}});
        }
        party_->reset();
        res_.clear();
    }
    cout << "nestloop comm: " << party_->get_comm() << endl;
    party_->clear_comm();
}

void NestLoopJoin::join_simd() {
    cout << "===== Nest Loop Join =====" << endl;
    party_->reset();
    res_.clear();
    circ_ = party_->get_circuit(S_BOOL);

    auto k1 = new uint32_t[size_c_];
    auto d1 = new uint32_t[size_c_];
    auto k2 = new uint32_t[size_c_];
    auto d2 = new uint32_t[size_c_];
    auto zero_array = new uint32_t[size_c_];
    for (int i = 0; i < size_c_; i++) {
        k2[i] = relation_[i].k;
        d2[i] = relation_[i].d;
        zero_array[i] = 0;
    }

    for (int i1 = 0; i1 < size_s_; i1++) {
        party_->reset();
        for (int i = 0; i < size_c_; i++) {
            k1[i] = relation_[i1].k;
            d1[i] = relation_[i1].d;
        }
        auto s_k1 = circ_->PutSIMDINGate(size_c_, k1, 32, SERVER);
        auto s_d1 = circ_->PutSIMDINGate(size_c_, d1, 32, SERVER);
        auto s_k2 = circ_->PutSIMDINGate(size_c_, k2, 32, CLIENT);
        auto s_d2 = circ_->PutSIMDINGate(size_c_, d2, 32, CLIENT);
        auto s_eq = circ_->PutEQGate(s_k1, s_k2);
        auto s_out_d1 = circ_->PutSIMDINGate(size_c_, zero_array, 32, SERVER);
        auto s_out_d2 = circ_->PutSIMDINGate(size_c_, zero_array, 32, SERVER);
        auto s_out_k = circ_->PutSIMDINGate(size_c_, zero_array, 32, SERVER);
        s_out_k = circ_->PutMUXGate(s_k1, s_out_k, s_eq);
        s_out_d1 = circ_->PutMUXGate(s_d1, s_out_d1, s_eq);
        s_out_d2 = circ_->PutMUXGate(s_d2, s_out_d2, s_eq);
        
        if (SHARED) {
            s_out_k = circ_->PutSharedOUTGate(s_out_k);
            s_out_d1 = circ_->PutSharedOUTGate(s_out_d1);
            s_out_d2 = circ_->PutSharedOUTGate(s_out_d2);
            s_eq = circ_->PutSharedOUTGate(s_eq);
        }
        else {
            s_out_k = circ_->PutOUTGate(s_out_k, ALL);
            s_out_d1 = circ_->PutOUTGate(s_out_d1, ALL);
            s_out_d2 = circ_->PutOUTGate(s_out_d2, ALL);
            s_eq = circ_->PutOUTGate(s_eq, ALL);
        }
        
        party_->exec();

        uint32_t *out_k, *out_d1, *out_d2, *out_eq;
        uint32_t nvals, bitlen;
        s_out_k->get_clear_value_vec(&out_k, &nvals, &bitlen);
        s_out_d1->get_clear_value_vec(&out_d1, &nvals, &bitlen);
        s_out_d2->get_clear_value_vec(&out_d2, &nvals, &bitlen);
        s_eq->get_clear_value_vec(&out_eq, &nvals, &bitlen);
        // for (int i = 0; i < res_.size(); i++) {

        // }
        for (int i = 0; i < size_c_; i++) {
            res_val_.push_back({out_k[i], out_d1[i], out_d2[i], {(uint32_t)out_eq[i]}});
        }
    }
    delete[] k1;
    delete[] k2;
    delete[] d1;
    delete[] d2;
    delete[] zero_array;
}

void NestLoopJoin::theta_join_simd() {
    cout << "===== Nest Loop Join =====" << endl;
    party_->reset();
    res_.clear();
    circ_ = party_->get_circuit(S_BOOL);

    auto k1 = new uint32_t[size_c_];
    auto d1 = new uint32_t[size_c_];
    auto k2 = new uint32_t[size_c_];
    auto d2 = new uint32_t[size_c_];
    auto zero_array = new uint32_t[size_c_];
    for (int i = 0; i < size_c_; i++) {
        k2[i] = relation_[i].k;
        d2[i] = relation_[i].d;
        zero_array[i] = 0;
    }

    for (int i1 = 0; i1 < size_s_; i1++) {
        party_->reset();
        for (int i = 0; i < size_c_; i++) {
            k1[i] = relation_[i1].k;
            d1[i] = relation_[i1].d;
        }
        auto s_k1 = circ_->PutSIMDINGate(size_c_, k1, 32, SERVER);
        auto s_d1 = circ_->PutSIMDINGate(size_c_, d1, 32, SERVER);
        auto s_k2 = circ_->PutSIMDINGate(size_c_, k2, 32, CLIENT);
        auto s_d2 = circ_->PutSIMDINGate(size_c_, d2, 32, CLIENT);
        auto s_lt = circ_->PutGTGate(s_k2, s_k1);
        auto s_out_d1 = circ_->PutSIMDINGate(size_c_, zero_array, 32, SERVER);
        auto s_out_d2 = circ_->PutSIMDINGate(size_c_, zero_array, 32, SERVER);
        auto s_out_k = circ_->PutSIMDINGate(size_c_, zero_array, 32, SERVER);
        s_out_k = circ_->PutMUXGate(s_k1, s_out_k, s_lt);
        s_out_d1 = circ_->PutMUXGate(s_d1, s_out_d1, s_lt);
        s_out_d2 = circ_->PutMUXGate(s_d2, s_out_d2, s_lt);
        
        if (SHARED) {
            s_out_k = circ_->PutSharedOUTGate(s_out_k);
            s_out_d1 = circ_->PutSharedOUTGate(s_out_d1);
            s_out_d2 = circ_->PutSharedOUTGate(s_out_d2);
            s_lt = circ_->PutSharedOUTGate(s_lt);
        }
        else {
            s_out_k = circ_->PutOUTGate(s_out_k, ALL);
            s_out_d1 = circ_->PutOUTGate(s_out_d1, ALL);
            s_out_d2 = circ_->PutOUTGate(s_out_d2, ALL);
            s_lt = circ_->PutOUTGate(s_lt, ALL);
        }
        
        party_->exec();

        uint32_t *out_k, *out_d1, *out_d2, *out_lt;
        uint32_t nvals, bitlen;
        s_out_k->get_clear_value_vec(&out_k, &nvals, &bitlen);
        s_out_d1->get_clear_value_vec(&out_d1, &nvals, &bitlen);
        s_out_d2->get_clear_value_vec(&out_d2, &nvals, &bitlen);
        s_lt->get_clear_value_vec(&out_lt, &nvals, &bitlen);

        for (int i = 0; i < size_c_; i++) {
            if (out_lt[i] == 1) {
                cout << out_k[i] << " " << out_d1[i] << " " << out_d2[i] << endl;
            }
            res_val_.push_back({out_k[i], out_d1[i], out_d2[i], {(uint32_t)out_lt[i]}});
        }
    }
    delete[] k1;
    delete[] k2;
    delete[] d1;
    delete[] d2;
    delete[] zero_array;
}

void NestLoopJoin::write(share* loc, share* k, share* d1, share* d2) {
    auto s_loc = circ_->PutRepeaterGate(size_res_, loc);
    auto s_new_k = circ_->PutRepeaterGate(size_res_, k);
    auto s_new_d1 = circ_->PutRepeaterGate(size_res_, d1);
    auto s_new_d2 = circ_->PutRepeaterGate(size_res_, d2);
    auto s_index = circ_->PutSIMDINGate(size_res_, res_index_, 32, SERVER);
    auto s_k = circ_->PutSharedSIMDINGate(size_res_, res_k_, 32);
    auto s_d1 = circ_->PutSharedSIMDINGate(size_res_, res_d1_, 32);
    auto s_d2 = circ_->PutSharedSIMDINGate(size_res_, res_d2_, 32);
    auto s_eq = circ_->PutEQGate(s_index, s_loc);
    s_k = circ_->PutMUXGate(s_new_k, s_k, s_eq);
    s_d1 = circ_->PutMUXGate(s_new_d1, s_d1, s_eq);
    s_d2 = circ_->PutMUXGate(s_new_d2, s_d2, s_eq);
    s_k = circ_->PutSharedOUTGate(s_k);
    s_d1 = circ_->PutSharedOUTGate(s_d1);
    s_d2 = circ_->PutSharedOUTGate(s_d2);
    party_->exec();
    uint32_t nvals, bitlen;
    s_k->get_clear_value_vec(&res_k_, &bitlen, &nvals);
    s_d1->get_clear_value_vec(&res_d1_, &bitlen, &nvals);
    s_d2->get_clear_value_vec(&res_d2_, &bitlen, &nvals);
}

void NestLoopJoin::join_oram() {
    if (size_res_ == 0) {
        cout << "please set res_size!" << endl;
        exit(0);
    }
    cout << "===== Nest Loop Join =====" << endl;
    party_->reset();
    res_.clear();
    circ_ = party_->get_circuit(S_BOOL);
    res_index_ = new uint32_t[size_res_];
    res_k_ = new uint32_t[size_res_];
    res_d1_ = new uint32_t[size_res_];
    res_d2_ = new uint32_t[size_res_];
    for (int i = 0; i < size_res_; i++) {
        res_index_[i] = i;
        res_k_[i] = 0;
        res_d1_[i] = 0;
        res_d2_[i] = 0;
    }
    
    for (int i1 = 0; i1 < size_s_; i1++) {
        cout << i1 << endl;
        for (int i2 = 0; i2 < size_c_; i2++) {
            party_->reset();
            auto s_k1 = circ_->PutINGate(relation_[i1].k, 32, SERVER);
            auto s_d1 = circ_->PutINGate(relation_[i1].d, 32, SERVER);
            auto s_k2 = circ_->PutINGate(relation_[i2].k, 32, CLIENT);
            auto s_d2 = circ_->PutINGate(relation_[i2].d, 32, CLIENT);
            auto s_zero = circ_->PutCONSGate(ZERO, 32);
            auto s_one = circ_->PutCONSGate(ONE, 32);
            auto s_eq = circ_->PutEQGate(s_k1, s_k2);
            auto s_loc = circ_->PutSharedINGate(cur_loc_, 32);
            auto s_loc_inc = circ_->PutADDGate(s_loc, s_one);
            auto s_new_loc = circ_->PutMUXGate(s_loc_inc, s_loc, s_eq);
            s_new_loc = circ_->PutSharedOUTGate(s_new_loc);
            s_loc = circ_->PutMUXGate(s_loc_inc, s_zero, s_eq);
            write(s_loc, s_k1, s_d1, s_d2);
            cur_loc_ = s_new_loc->get_clear_value<uint32_t>();
        }
    }
    party_->reset();
    auto s_k = circ_->PutSharedSIMDINGate(size_res_, res_k_, 32);
    auto s_d1 = circ_->PutSharedSIMDINGate(size_res_, res_d1_, 32);
    auto s_d2 = circ_->PutSharedSIMDINGate(size_res_, res_d2_, 32);
    s_k = circ_->PutOUTGate(s_k, ALL);
    s_d1 = circ_->PutOUTGate(s_d1, ALL);
    s_d2 = circ_->PutOUTGate(s_d2, ALL);
    party_->exec();
    uint32_t nvals, bitlen;
    s_k->get_clear_value_vec(&res_k_, &bitlen, &nvals);
    s_d1->get_clear_value_vec(&res_d1_, &bitlen, &nvals);
    s_d2->get_clear_value_vec(&res_d2_, &bitlen, &nvals);
    for (int i = 1; i < size_res_; i++) {
        if (res_k_[i] == 0) {
            break;
        }
        cout << "(" << i << ") " << res_k_[i] << " " << res_d1_[i] << " " << res_d2_[i] << endl;
    }
    
    cout << "nestloop comm: " << party_->get_comm() << endl;
    party_->clear_comm();
}
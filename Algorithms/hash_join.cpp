#include "hash_join.h"

void HashJoin::join() {
    cout << "===== Hash Join =====" << endl;
    party_->reset();
    circ_ = party_->get_circuit(S_BOOL);
    int cnt = 1;
    for (uint32_t i2 = 0; i2 < size_c_; i2++) {
        party_->reset();
        auto s_k2_loc = circ_->PutINGate(equal_hash(relation_[i2].k), 32, CLIENT);
        share *s_k1, *s_d1;
        oram_linear(s_k2_loc, &s_k1, &s_d1);
        auto s_k2 = circ_->PutINGate(relation_[i2].k, 32, CLIENT);
        auto s_d2 = circ_->PutINGate(relation_[i2].d, 32, CLIENT);
        auto s_eq = circ_->PutEQGate(s_k1, s_k2);
        auto s_zero = circ_->PutCONSGate(ZERO, 32);
        auto s_out_k = circ_->PutMUXGate(s_k2, s_zero, s_eq);
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
        party_->exec();
        auto eq = s_eq->get_clear_value<uint32_t>();
        auto out_k = s_out_k->get_clear_value<uint32_t>();
        auto out_d1 = s_out_d1->get_clear_value<uint32_t>();
        auto out_d2 = s_out_d2->get_clear_value<uint32_t>();
        if (SHARED || eq) {
            cout << "(" << cnt << ") " << out_k << " " << out_d1 << " " << out_d2 << endl;
            cnt++;
        }
        res_val_.push_back({out_k, out_d1, out_d2, {eq}});
    }
    cout << "hash comm: " << party_->get_comm() << endl;
    party_->clear_comm();
}

void HashJoin::oram_linear(share* s_select, share** ret_k, share** ret_d) {
    *ret_k = circ_->PutINGate(NIL, 32, SERVER);
    *ret_d = circ_->PutINGate(NIL, 32, SERVER);
    for (uint32_t i = 0; i < table_size_; i++) {
        auto s_k = circ_->PutINGate(hash_table_key_[i], 32, SERVER);
        auto s_d = circ_->PutINGate(hash_table_data_[i], 32, SERVER);
        auto s_index = circ_->PutINGate(i, 32, SERVER);
        auto s_eq = circ_->PutEQGate(s_index, s_select);
        *ret_k = circ_->PutMUXGate(s_k, *ret_k, s_eq);
        *ret_d = circ_->PutMUXGate(s_d, *ret_d, s_eq);
    }
}

// void HashJoin::par_oram_linear() {
//     TODO
// }

void HashJoin::construct_hash_table() {
    for (int i = 0; i < size_s_; i++) {
        auto r = relation_[i];
        hash_table_key_[equal_hash(r.k)] = r.k;
        hash_table_data_[equal_hash(r.k)] = r.d;
    }
}
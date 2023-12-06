#include "obliv_join.h"
#define PARALLEL

void aug_table_cmp1(vector<SharedTuple>& table, bool asc, int index1, int index2, Circuit* circ) {
    auto e1 = table[index1], e2 = table[index2];
    auto s_asc = circ->PutINGate((uint32_t)asc, 32, SERVER);

    auto k_gt = circ->PutGTGate(e1.k, e2.k);
	auto k_eq = circ->PutEQGate(e1.k, e2.k);
	auto tid_gt = circ->PutGTGate(e1.inds[0], e2.inds[0]);
	tid_gt = circ->PutANDGate(k_eq, tid_gt);

    auto cond = circ->PutADDGate(k_gt, tid_gt);
    cond = circ->PutEQGate(s_asc, cond);

    table[index1].cond_update(e2, cond, circ);
    table[index2].cond_update(e1, cond, circ);
}

void aug_table_cmp2(vector<SharedTuple>& table, bool asc, int index1, int index2, Circuit* circ) {
    auto e1 = table[index1], e2 = table[index2];
    auto s_asc = circ->PutINGate((uint32_t)asc, 32, SERVER);

    auto tid_gt = circ->PutGTGate(e1.inds[0], e2.inds[0]);
	auto tid_eq = circ->PutEQGate(e1.inds[0], e2.inds[0]);
	auto k_gt = circ->PutGTGate(e1.k, e2.k);
	k_gt = circ->PutANDGate(tid_eq, k_gt);
    auto cond = circ->PutADDGate(k_gt, tid_gt);
    cond = circ->PutEQGate(s_asc, cond);

    table[index1].cond_update(e2, cond, circ);
    table[index2].cond_update(e1, cond, circ);
}

void exp_table_cmp(vector<SharedTuple>& table, bool asc, int index1, int index2, Circuit* circ) {
    uint32_t NIL = 0;
    auto s_nil = circ->PutCONSGate(NIL, 32);
    auto e1 = table[index1], e2 = table[index2];
    auto s_asc = circ->PutINGate((uint32_t)asc, 32, SERVER);

	auto k1_eq_nil = circ->PutEQGate(e1.k, s_nil);
	auto k2_eq_nil = circ->PutEQGate(e2.k, s_nil);
	auto k_gt = circ->PutGTGate(k1_eq_nil, k2_eq_nil);
	auto k_eq = circ->PutEQGate(k1_eq_nil, k2_eq_nil);
	auto f_gt = circ->PutGTGate(e1.inds[2], e2.inds[2]);
	f_gt = circ->PutANDGate(k_eq, f_gt);

	auto cond = circ->PutADDGate(f_gt, k_gt);
    cond = circ->PutEQGate(s_asc, cond);
    table[index1].cond_update(e2, cond, circ);
    table[index2].cond_update(e1, cond, circ);
}

void align_table_cmp(vector<SharedTuple>& table, bool asc, int index1, int index2, Circuit* circ) {
    auto e1 = table[index1], e2 = table[index2];
    auto s_asc = circ->PutINGate((uint32_t)asc, 32, SERVER);

	auto k_gt = circ->PutGTGate(e1.k, e2.k);
	auto k_eq = circ->PutEQGate(e1.k, e2.k);
	auto ii_gt = circ->PutGTGate(e1.inds[0], e2.inds[0]);
	ii_gt = circ->PutANDGate(k_eq, ii_gt);

    auto cond = circ->PutADDGate(k_gt, ii_gt);
    cond = circ->PutEQGate(s_asc, cond);

    table[index1].cond_update(e2, cond, circ);
    table[index2].cond_update(e1, cond, circ);
}

void par_aug_table_cmp1(Party* party, vector<Tuple>& in1, vector<Tuple>& in2, vector<Tuple>& small, vector<Tuple>& big) {
    vector<uint32_t> k1, k2, d1, d2, tid1, tid2;
    uint32_t len = in1.size();
    for (int i = 0; i < len; i++) {
        k1.push_back(in1[i].k);
        k2.push_back(in2[i].k);
        d1.push_back(in1[i].d);
        d2.push_back(in2[i].d);
        tid1.push_back(in1[i].inds[0]);
        tid2.push_back(in2[i].inds[0]);
    }
    party->reset();
    auto circ = party->get_circuit(S_BOOL);
    auto s_k1 = circ->PutSharedSIMDINGate(len, k1.data(), 32);
    auto s_k2 = circ->PutSharedSIMDINGate(len, k2.data(), 32);
    auto s_d1 = circ->PutSharedSIMDINGate(len, d1.data(), 32);
    auto s_d2 = circ->PutSharedSIMDINGate(len, d2.data(), 32);
    auto s_tid1 = circ->PutSharedSIMDINGate(len, tid1.data(), 32);
    auto s_tid2 = circ->PutSharedSIMDINGate(len, tid2.data(), 32);

    auto xor_k = circ->PutXORGate(s_k1, s_k2);
    auto xor_d = circ->PutXORGate(s_d1, s_d2);
    auto xor_tid = circ->PutXORGate(s_tid1, s_tid2);

    auto k_gt = circ->PutGTGate(s_k1, s_k2);
    auto k_eq = circ->PutEQGate(s_k1, s_k2);
    auto tid_gt = circ->PutGTGate(s_tid1, s_tid2);
    tid_gt = circ->PutANDGate(tid_gt, k_eq);
    auto cond = circ->PutADDGate(tid_gt, k_gt);
    
    auto s_small_k = circ->PutMUXGate(s_k2, s_k1, cond);
    auto s_small_d = circ->PutMUXGate(s_d2, s_d1, cond);
    auto s_small_tid = circ->PutMUXGate(s_tid2, s_tid1, cond);
    auto s_big_k = circ->PutXORGate(s_small_k, xor_k);
    auto s_big_d = circ->PutXORGate(s_small_d, xor_d);
    auto s_big_tid = circ->PutXORGate(s_small_tid, xor_tid);

    s_small_k = circ->PutSharedOUTGate(s_small_k);
    s_small_d = circ->PutSharedOUTGate(s_small_d);
    s_small_tid = circ->PutSharedOUTGate(s_small_tid);
    s_big_k = circ->PutSharedOUTGate(s_big_k);
    s_big_d = circ->PutSharedOUTGate(s_big_d);
    s_big_tid = circ->PutSharedOUTGate(s_big_tid);
    
    party->exec();
    
    uint32_t *small_k, *small_d, *small_tid, *big_k, *big_d, *big_tid;
    uint32_t nvals, bitlen;
    s_small_k->get_clear_value_vec(&small_k, &bitlen, &nvals);
    s_small_d->get_clear_value_vec(&small_d, &bitlen, &nvals);
    s_small_tid->get_clear_value_vec(&small_tid, &bitlen, &nvals);
    s_big_k->get_clear_value_vec(&big_k, &bitlen, &nvals);
    s_big_d->get_clear_value_vec(&big_d, &bitlen, &nvals);
    s_big_tid->get_clear_value_vec(&big_tid, &bitlen, &nvals);

    small.clear();
    big.clear();
    for (int i = 0; i < len; i++) {
        vector<uint32_t> inds = {small_tid[i], 0, 0};
        small.push_back(Tuple{small_k[i], small_d[i], inds});
        inds = {big_tid[i], 0, 0};
        big.push_back(Tuple{big_k[i], big_d[i], inds});
    }
    delete[] small_k;
    delete[] small_d;
    delete[] small_tid;
    delete[] big_k;
    delete[] big_d;
    delete[] big_tid;
}

void par_aug_table_cmp2(Party* party, vector<Tuple>& in1, vector<Tuple>& in2, vector<Tuple>& small, vector<Tuple>& big) {
    vector<uint32_t> k1, k2, d1, d2, tid1, tid2, alphaone1, alphaone2, alphatwo1, alphatwo2;
    uint32_t len = in1.size();
    for (int i = 0; i < len; i++) {
        k1.push_back(in1[i].k);
        k2.push_back(in2[i].k);
        d1.push_back(in1[i].d);
        d2.push_back(in2[i].d);
        tid1.push_back(in1[i].inds[0]);
        tid2.push_back(in2[i].inds[0]);
        alphaone1.push_back(in1[i].inds[1]);
        alphaone2.push_back(in2[i].inds[1]);
        alphatwo1.push_back(in1[i].inds[2]);
        alphatwo2.push_back(in2[i].inds[2]);
    }
    party->reset();
    auto circ = party->get_circuit(S_BOOL);
    auto s_k1 = circ->PutSharedSIMDINGate(len, k1.data(), 32);
    auto s_k2 = circ->PutSharedSIMDINGate(len, k2.data(), 32);
    auto s_d1 = circ->PutSharedSIMDINGate(len, d1.data(), 32);
    auto s_d2 = circ->PutSharedSIMDINGate(len, d2.data(), 32);
    auto s_tid1 = circ->PutSharedSIMDINGate(len, tid1.data(), 32);
    auto s_tid2 = circ->PutSharedSIMDINGate(len, tid2.data(), 32);
    auto s_alphaone1 = circ->PutSharedSIMDINGate(len, alphaone1.data(), 32);
    auto s_alphaone2 = circ->PutSharedSIMDINGate(len, alphaone2.data(), 32);
    auto s_alphatwo1 = circ->PutSharedSIMDINGate(len, alphatwo1.data(), 32);
    auto s_alphatwo2 = circ->PutSharedSIMDINGate(len, alphatwo2.data(), 32);

    auto xor_k = circ->PutXORGate(s_k1, s_k2);
    auto xor_d = circ->PutXORGate(s_d1, s_d2);
    auto xor_tid = circ->PutXORGate(s_tid1, s_tid2);
    auto xor_alphaone = circ->PutXORGate(s_alphaone1, s_alphaone2);
    auto xor_alphatwo = circ->PutXORGate(s_alphatwo1, s_alphatwo2);
    
    auto tid_gt = circ->PutGTGate(s_tid1, s_tid2);
    auto tid_eq = circ->PutEQGate(s_tid1, s_tid2);
    auto k_gt = circ->PutGTGate(s_k1, s_k2);
    k_gt = circ->PutANDGate(tid_eq, k_gt);
    auto cond = circ->PutADDGate(tid_gt, k_gt);
    
    auto s_small_k = circ->PutMUXGate(s_k2, s_k1, cond);
    auto s_small_d = circ->PutMUXGate(s_d2, s_d1, cond);
    auto s_small_tid = circ->PutMUXGate(s_tid2, s_tid1, cond);
    auto s_small_alphaone = circ->PutMUXGate(s_alphaone2, s_alphaone1, cond);
    auto s_small_alphatwo = circ->PutMUXGate(s_alphatwo2, s_alphatwo1, cond);
    auto s_big_k = circ->PutXORGate(s_small_k, xor_k);
    auto s_big_d = circ->PutXORGate(s_small_d, xor_d);
    auto s_big_tid = circ->PutXORGate(s_small_tid, xor_tid);
    auto s_big_alphaone = circ->PutXORGate(s_small_alphaone, xor_alphaone);
    auto s_big_alphatwo = circ->PutXORGate(s_small_alphatwo, xor_alphatwo);

    s_small_k = circ->PutSharedOUTGate(s_small_k);
    s_small_d = circ->PutSharedOUTGate(s_small_d);
    s_small_tid = circ->PutSharedOUTGate(s_small_tid);
    s_small_alphaone = circ->PutSharedOUTGate(s_small_alphaone);
    s_small_alphatwo = circ->PutSharedOUTGate(s_small_alphatwo);
    s_big_k = circ->PutSharedOUTGate(s_big_k);
    s_big_d = circ->PutSharedOUTGate(s_big_d);
    s_big_tid = circ->PutSharedOUTGate(s_big_tid);
    s_big_alphaone = circ->PutSharedOUTGate(s_big_alphaone);
    s_big_alphatwo = circ->PutSharedOUTGate(s_big_alphatwo);

    party->exec();
    
    uint32_t *small_k, *small_d, *small_tid, *big_k, *big_d, *big_tid;
    uint32_t *small_alphaone, *small_alphatwo, *big_alphaone, *big_alphatwo;
    uint32_t nvals, bitlen;
    s_small_k->get_clear_value_vec(&small_k, &bitlen, &nvals);
    s_small_d->get_clear_value_vec(&small_d, &bitlen, &nvals);
    s_small_tid->get_clear_value_vec(&small_tid, &bitlen, &nvals);
    s_small_alphaone->get_clear_value_vec(&small_alphaone, &bitlen, &nvals);
    s_small_alphatwo->get_clear_value_vec(&small_alphatwo, &bitlen, &nvals);
    s_big_k->get_clear_value_vec(&big_k, &bitlen, &nvals);
    s_big_d->get_clear_value_vec(&big_d, &bitlen, &nvals);
    s_big_tid->get_clear_value_vec(&big_tid, &bitlen, &nvals);
    s_big_alphaone->get_clear_value_vec(&big_alphaone, &bitlen, &nvals);
    s_big_alphatwo->get_clear_value_vec(&big_alphatwo, &bitlen, &nvals);

    small.clear();
    big.clear();
    for (int i = 0; i < len; i++) {
        vector<uint32_t> inds = {small_tid[i], small_alphaone[i], small_alphatwo[i]};
        small.push_back(Tuple{small_k[i], small_d[i], inds});
        inds = {big_tid[i], big_alphaone[i], big_alphatwo[i]};
        big.push_back(Tuple{big_k[i], big_d[i], inds});
    }
    delete[] small_k;
    delete[] small_d;
    delete[] small_tid;
    delete[] big_k;
    delete[] big_d;
    delete[] big_tid;
    delete[] small_alphaone;
    delete[] small_alphatwo;
    delete[] big_alphaone;
    delete[] big_alphatwo;
}

void par_exp_table_cmp(Party* party, vector<Tuple>& in1, vector<Tuple>& in2, vector<Tuple>& small, vector<Tuple>& big) {
    vector<uint32_t> k1, k2, d1, d2, alpha1, alpha2, other1, other2, f1, f2, nil;
    uint32_t len = in1.size();
    for (int i = 0; i < len; i++) {
        k1.push_back(in1[i].k);
        k2.push_back(in2[i].k);
        d1.push_back(in1[i].d);
        d2.push_back(in2[i].d);
        alpha1.push_back(in1[i].inds[0]);
        alpha2.push_back(in2[i].inds[0]);
        other1.push_back(in1[i].inds[1]);
        other2.push_back(in2[i].inds[1]);
        f1.push_back(in1[i].inds[2]);
        f2.push_back(in2[i].inds[2]);
        nil.push_back(0);
    }
    party->reset();
    auto circ = party->get_circuit(S_BOOL);
    auto s_k1 = circ->PutSharedSIMDINGate(len, k1.data(), 32);
    auto s_k2 = circ->PutSharedSIMDINGate(len, k2.data(), 32);
    auto s_d1 = circ->PutSharedSIMDINGate(len, d1.data(), 32);
    auto s_d2 = circ->PutSharedSIMDINGate(len, d2.data(), 32);
    auto s_alpha1 = circ->PutSharedSIMDINGate(len, alpha1.data(), 32);
    auto s_alpha2 = circ->PutSharedSIMDINGate(len, alpha2.data(), 32);
    auto s_other1 = circ->PutSharedSIMDINGate(len, other1.data(), 32);
    auto s_other2 = circ->PutSharedSIMDINGate(len, other2.data(), 32);
    auto s_f1 = circ->PutSharedSIMDINGate(len, f1.data(), 32);
    auto s_f2 = circ->PutSharedSIMDINGate(len, f2.data(), 32);
    auto s_nil = circ->PutSharedSIMDINGate(len, nil.data(), 32);

    auto xor_k = circ->PutXORGate(s_k1, s_k2);
    auto xor_d = circ->PutXORGate(s_d1, s_d2);
    auto xor_alpha = circ->PutXORGate(s_alpha1, s_alpha2);
    auto xor_other = circ->PutXORGate(s_other1, s_other2);
    auto xor_f = circ->PutXORGate(s_f1, s_f2);

    auto k1_eq_nil = circ->PutEQGate(s_k1, s_nil);
    auto k2_eq_nil = circ->PutEQGate(s_k2, s_nil);
    auto k_gt = circ->PutGTGate(k1_eq_nil, k2_eq_nil);
	auto k_eq = circ->PutEQGate(k1_eq_nil, k2_eq_nil);
	auto f_gt = circ->PutGTGate(s_f1, s_f2);
	f_gt = circ->PutANDGate(k_eq, f_gt);
	auto cond = circ->PutADDGate(f_gt, k_gt);
    
    auto s_small_k = circ->PutMUXGate(s_k2, s_k1, cond);
    auto s_small_d = circ->PutMUXGate(s_d2, s_d1, cond);
    auto s_small_alpha = circ->PutMUXGate(s_alpha2, s_alpha1, cond);
    auto s_small_other = circ->PutMUXGate(s_other2, s_other1, cond);
    auto s_small_f = circ->PutMUXGate(s_f2, s_f1, cond);
    auto s_big_k = circ->PutXORGate(s_small_k, xor_k);
    auto s_big_d = circ->PutXORGate(s_small_d, xor_d);
    auto s_big_alpha = circ->PutXORGate(s_small_alpha, xor_alpha);
    auto s_big_other = circ->PutXORGate(s_small_other, xor_other);
    auto s_big_f = circ->PutXORGate(s_small_f, xor_f);

    s_small_k = circ->PutSharedOUTGate(s_small_k);
    s_small_d = circ->PutSharedOUTGate(s_small_d);
    s_small_alpha = circ->PutSharedOUTGate(s_small_alpha);
    s_small_other = circ->PutSharedOUTGate(s_small_other);
    s_small_f = circ->PutSharedOUTGate(s_small_f);
    s_big_k = circ->PutSharedOUTGate(s_big_k);
    s_big_d = circ->PutSharedOUTGate(s_big_d);
    s_big_alpha = circ->PutSharedOUTGate(s_big_alpha);
    s_big_other = circ->PutSharedOUTGate(s_big_other);
    s_big_f = circ->PutSharedOUTGate(s_big_f);
    
    party->exec();
    
    uint32_t *small_k, *small_d, *small_alpha, *small_other, *small_f;
    uint32_t *big_k, *big_d, *big_alpha, *big_other, *big_f;
    uint32_t nvals, bitlen;
    s_small_k->get_clear_value_vec(&small_k, &bitlen, &nvals);
    s_small_d->get_clear_value_vec(&small_d, &bitlen, &nvals);
    s_small_alpha->get_clear_value_vec(&small_alpha, &bitlen, &nvals);
    s_small_other->get_clear_value_vec(&small_other, &bitlen, &nvals);
    s_small_f->get_clear_value_vec(&small_f, &bitlen, &nvals);
    s_big_k->get_clear_value_vec(&big_k, &bitlen, &nvals);
    s_big_d->get_clear_value_vec(&big_d, &bitlen, &nvals);
    s_big_alpha->get_clear_value_vec(&big_alpha, &bitlen, &nvals);
    s_big_other->get_clear_value_vec(&big_other, &bitlen, &nvals);
    s_big_f->get_clear_value_vec(&big_f, &bitlen, &nvals);

    small.clear();
    big.clear();
    for (int i = 0; i < len; i++) {
        vector<uint32_t> inds = {small_alpha[i], small_other[i], small_f[i]};
        small.push_back(Tuple{small_k[i], small_d[i], inds});
        inds = {big_alpha[i], big_other[i], big_f[i]};
        big.push_back(Tuple{big_k[i], big_d[i], inds});
    }
    delete[] small_k;
    delete[] small_d;
    delete[] small_alpha;
    delete[] small_other;
    delete[] small_f;
    delete[] big_k;
    delete[] big_d;
    delete[] big_alpha;
    delete[] big_other;
    delete[] big_f;
}

void par_exp_table_cmp_tmp(Party* party, vector<Tuple>& in1, vector<Tuple>& in2, vector<Tuple>& small, vector<Tuple>& big) {
    vector<uint32_t> k1, k2, d1, d2, alpha1, alpha2, other1, other2, f1, f2, nil;
    uint32_t len = in1.size();
    for (int i = 0; i < len; i++) {
        k1.push_back(in1[i].k);
        k2.push_back(in2[i].k);
        d1.push_back(in1[i].d);
        d2.push_back(in2[i].d);
        alpha1.push_back(in1[i].inds[0]);
        alpha2.push_back(in2[i].inds[0]);
        other1.push_back(in1[i].inds[1]);
        other2.push_back(in2[i].inds[1]);
        f1.push_back(in1[i].inds[2]);
        f2.push_back(in2[i].inds[2]);
        nil.push_back(0);
    }
    party->reset();
    auto circ = party->get_circuit(S_BOOL);
    auto s_k1 = circ->PutSIMDINGate(len, k1.data(), 32, SERVER);
    auto s_k2 = circ->PutSIMDINGate(len, k2.data(), 32, SERVER);
    auto s_d1 = circ->PutSIMDINGate(len, d1.data(), 32, SERVER);
    auto s_d2 = circ->PutSIMDINGate(len, d2.data(), 32, SERVER);
    auto s_alpha1 = circ->PutSIMDINGate(len, alpha1.data(), 32, SERVER);
    auto s_alpha2 = circ->PutSIMDINGate(len, alpha2.data(), 32, SERVER);
    auto s_other1 = circ->PutSIMDINGate(len, other1.data(), 32, SERVER);
    auto s_other2 = circ->PutSIMDINGate(len, other2.data(), 32, SERVER);
    auto s_f1 = circ->PutSIMDINGate(len, f1.data(), 32, SERVER);
    auto s_f2 = circ->PutSIMDINGate(len, f2.data(), 32, SERVER);
    auto s_nil = circ->PutSIMDINGate(len, nil.data(), 32, SERVER);

    auto xor_k = circ->PutXORGate(s_k1, s_k2);
    auto xor_d = circ->PutXORGate(s_d1, s_d2);
    auto xor_alpha = circ->PutXORGate(s_alpha1, s_alpha2);
    auto xor_other = circ->PutXORGate(s_other1, s_other2);
    auto xor_f = circ->PutXORGate(s_f1, s_f2);

    auto k1_eq_nil = circ->PutEQGate(s_k1, s_nil);
    auto k2_eq_nil = circ->PutEQGate(s_k2, s_nil);
    auto k_gt = circ->PutGTGate(k1_eq_nil, k2_eq_nil);
	auto k_eq = circ->PutEQGate(k1_eq_nil, k2_eq_nil);
	auto f_gt = circ->PutGTGate(s_f1, s_f2);
	f_gt = circ->PutANDGate(k_eq, f_gt);
	auto cond = circ->PutADDGate(f_gt, k_gt);
    
    auto s_small_k = circ->PutMUXGate(s_k2, s_k1, cond);
    auto s_small_d = circ->PutMUXGate(s_d2, s_d1, cond);
    auto s_small_alpha = circ->PutMUXGate(s_alpha2, s_alpha1, cond);
    auto s_small_other = circ->PutMUXGate(s_other2, s_other1, cond);
    auto s_small_f = circ->PutMUXGate(s_f2, s_f1, cond);
    auto s_big_k = circ->PutXORGate(s_small_k, xor_k);
    auto s_big_d = circ->PutXORGate(s_small_d, xor_d);
    auto s_big_alpha = circ->PutXORGate(s_small_alpha, xor_alpha);
    auto s_big_other = circ->PutXORGate(s_small_other, xor_other);
    auto s_big_f = circ->PutXORGate(s_small_f, xor_f);

    s_small_k = circ->PutOUTGate(s_small_k, ALL);
    s_small_d = circ->PutOUTGate(s_small_d, ALL);
    s_small_alpha = circ->PutOUTGate(s_small_alpha, ALL);
    s_small_other = circ->PutOUTGate(s_small_other, ALL);
    s_small_f = circ->PutOUTGate(s_small_f, ALL);
    s_big_k = circ->PutOUTGate(s_big_k, ALL);
    s_big_d = circ->PutOUTGate(s_big_d, ALL);
    s_big_alpha = circ->PutOUTGate(s_big_alpha, ALL);
    s_big_other = circ->PutOUTGate(s_big_other, ALL);
    s_big_f = circ->PutOUTGate(s_big_f, ALL);
    
    party->exec();
    
    uint32_t *small_k, *small_d, *small_alpha, *small_other, *small_f;
    uint32_t *big_k, *big_d, *big_alpha, *big_other, *big_f;
    uint32_t nvals, bitlen;
    s_small_k->get_clear_value_vec(&small_k, &bitlen, &nvals);
    s_small_d->get_clear_value_vec(&small_d, &bitlen, &nvals);
    s_small_alpha->get_clear_value_vec(&small_alpha, &bitlen, &nvals);
    s_small_other->get_clear_value_vec(&small_other, &bitlen, &nvals);
    s_small_f->get_clear_value_vec(&small_f, &bitlen, &nvals);
    s_big_k->get_clear_value_vec(&big_k, &bitlen, &nvals);
    s_big_d->get_clear_value_vec(&big_d, &bitlen, &nvals);
    s_big_alpha->get_clear_value_vec(&big_alpha, &bitlen, &nvals);
    s_big_other->get_clear_value_vec(&big_other, &bitlen, &nvals);
    s_big_f->get_clear_value_vec(&big_f, &bitlen, &nvals);

    small.clear();
    big.clear();
    for (int i = 0; i < len; i++) {
        vector<uint32_t> inds = {small_alpha[i], small_other[i], small_f[i]};
        small.push_back(Tuple{small_k[i], small_d[i], inds});
        inds = {big_alpha[i], big_other[i], big_f[i]};
        big.push_back(Tuple{big_k[i], big_d[i], inds});
    }
}

void par_align_table_cmp(Party* party, vector<Tuple>& in1, vector<Tuple>& in2, vector<Tuple>& small, vector<Tuple>& big) {
    par_aug_table_cmp1(party, in1, in2, small, big);
}

void OblivJoin::prepare_aug_table() {
    cout << "prepare_aug_table" << endl;
    party_->reset();
    circ_ = party_->get_circuit(S_BOOL);
    auto s_nil = circ_->PutINGate(NIL, 32, SERVER);
    for (int i = 0; i < size_aug_; i++) {
        share *k = s_nil, *d, *tid, *alpha1, *alpha2;
        alpha1 = circ_->PutINGate(ZERO, 32, SERVER);
        alpha2 = circ_->PutINGate(ZERO, 32, SERVER);
        if (i < size_s_) {
            k = circ_->PutINGate(relation_[i].k, 32, SERVER);
            d = circ_->PutINGate(relation_[i].d, 32, SERVER);
            tid = circ_->PutINGate(ONE, 32, SERVER);
        }
        else {
            k = circ_->PutINGate(relation_[i - size_s_].k, 32, CLIENT);
            d = circ_->PutINGate(relation_[i - size_s_].d, 32, CLIENT);
            tid = circ_->PutINGate(TWO, 32, CLIENT);
        }
        vector<share*> inds = {tid, alpha1, alpha2};
        aug_table_.push_back(SharedTuple{k, d, inds});
    }

#ifndef PARALLEL
    bitonic_sort<SharedTuple, aug_table_cmp1>(aug_table_, true, 0, size_aug_, circ_);
#else
    for (int i = 0; i < size_aug_; i++) {
        aug_table_[i].reveal_share(circ_);
    }
    party_->exec();
    vector<Tuple> aug_table_v;
    for (int i = 0; i < size_aug_; i++) {
        auto t = aug_table_[i].get_value(circ_);
        aug_table_v.push_back(t);
    }
    Tuple nv = Tuple::null_value();
    if (party_->get_role() == SERVER) {
        nv.k = 0x7fffffff;
    }
    BitonicSort bs1(party_, aug_table_v, nv);
    bs1.sort(par_aug_table_cmp1);
    aug_table_v = bs1.get_output();
    aug_table_.clear();
    for (int i = 0; i < size_aug_; i++) {
        aug_table_.push_back(SharedTuple{aug_table_v[i], circ_, true});
    }
#endif
    
    downward_scan();
    upward_scan();

#ifndef PARALLEL
    bitonic_sort<SharedTuple, aug_table_cmp2>(aug_table_, true, 0, size_aug_, circ_);
#else
    for (int i = 0; i < size_aug_; i++) {
        aug_table_[i].reveal_share(circ_);
    }
    party_->exec();
    aug_table_v.clear();
    for (int i = 0; i < size_aug_; i++) {
        auto t = aug_table_[i].get_value(circ_);
        aug_table_v.push_back(t);
    }
    if (party_->get_role() == SERVER) {
        nv.inds[0] = 2;
    }
    BitonicSort bs2(party_, aug_table_v, nv);
    bs2.sort(par_aug_table_cmp2);
    aug_table_v = bs2.get_output();
    aug_table_.clear();
    for (int i = 0; i < size_aug_; i++) {
        aug_table_.push_back(SharedTuple{aug_table_v[i], circ_, true});
    }
#endif
}

/*
tid	1		2
eq	inc/0	prev/inc
ne	1/0		1/1
*/
void OblivJoin::downward_scan() {
    auto s_zero = circ_->PutCONSGate(ZERO, 32);
    auto s_one = circ_->PutCONSGate(ONE, 32);
    aug_table_[0].inds[1] = s_one;
    for (int i = 1; i < size_aug_; i++) {
        auto k_eq = circ_->PutEQGate(aug_table_[i - 1].k, aug_table_[i].k);
        auto tid_eq_one = circ_->PutEQGate(aug_table_[i].inds[0], s_one);
        auto prev_alpha1 = aug_table_[i - 1].inds[1];
		auto prev_alpha1_inc = circ_->PutADDGate(prev_alpha1, s_one);
		auto alpha1_k_ne = circ_->PutMUXGate(s_one, s_zero, tid_eq_one);
		auto alpha1_k_eq = circ_->PutMUXGate(prev_alpha1_inc, prev_alpha1, tid_eq_one);
		aug_table_[i].inds[1] = circ_->PutMUXGate(alpha1_k_eq, alpha1_k_ne, k_eq);
		auto prev_alpha2_inc = circ_->PutADDGate(aug_table_[i - 1].inds[2], s_one);
		aug_table_[i].inds[2] = circ_->PutMUXGate(prev_alpha2_inc, s_one, k_eq);
		aug_table_[i].inds[2] = circ_->PutMUXGate(s_zero, aug_table_[i].inds[2], tid_eq_one);
    }
}

/*
tid	1			2
eq	prev/prev   self/prev
ne	self/0		self/self
*/
void OblivJoin::upward_scan() {
    auto s_zero = circ_->PutCONSGate(ZERO, 32);
    auto s_one = circ_->PutCONSGate(ONE, 32);
    auto tid_one = circ_->PutEQGate(aug_table_[size_aug_ - 1].inds[0], s_one);
	aug_table_[size_aug_ - 1].inds[2] = circ_->PutMUXGate(s_zero, aug_table_[size_aug_ - 1].inds[2], tid_one);
	for (int i = size_aug_ - 2; i >= 0; i--) {
		auto k_eq = circ_->PutEQGate(aug_table_[i + 1].k, aug_table_[i].k);
		auto tid_eq_one = circ_->PutEQGate(aug_table_[i].inds[0], s_one);
		auto eq_prev_ind = circ_->PutANDGate(k_eq, tid_eq_one);
		aug_table_[i].inds[1] = circ_->PutMUXGate(aug_table_[i + 1].inds[1], aug_table_[i].inds[1], eq_prev_ind);

		auto alpha2_k_eq = aug_table_[i + 1].inds[2];
		auto alpha2_k_ne = circ_->PutMUXGate(s_zero, aug_table_[i].inds[2], tid_eq_one);
		aug_table_[i].inds[2] = circ_->PutMUXGate(alpha2_k_eq, alpha2_k_ne, k_eq);
	}
}

void OblivJoin::split_table() {
    cout << "split_table" << endl;
    auto s_zero = circ_->PutCONSGate(ZERO, 32);
    auto s_one = circ_->PutCONSGate(ONE, 32);
    auto s_size = s_zero;
    for (int i = 0; i < size_aug_; i++) {
        auto tid_eq_one = circ_->PutEQGate(aug_table_[i].inds[0], s_one);
		auto s_val = circ_->PutMUXGate(aug_table_[i].inds[2], s_zero, tid_eq_one);
		s_size = circ_->PutADDGate(s_size, s_val);
    }
    s_size = circ_->PutOUTGate(s_size, ALL);

#ifdef DEBUG
    for (int i = 0; i < size_aug_; i++) {
        aug_table_[i].reveal(circ_);
    }
#else
    for (int i = 0; i < size_aug_; i++) {
        aug_table_[i].reveal_share(circ_);
    }
#endif

    party_->exec();
    size_res_ = s_size->get_clear_value<uint32_t>();
    cout << "size = " << size_res_ << endl;
    if (size_dummy_ != 0) {
        if (size_dummy_ < size_res_) {
            cout << "size error!" << endl;
            exit(0);
        }
        size_res_ = size_dummy_;
        cout << "padded size = " << size_res_ << endl;
    }
    for (int i = 0; i < size_s_; i++) {
        uint32_t k = aug_table_[i].k->get_clear_value<uint32_t>();
        uint32_t d = aug_table_[i].d->get_clear_value<uint32_t>();
        uint32_t alpha = aug_table_[i].inds[2]->get_clear_value<uint32_t>();
        uint32_t other = aug_table_[i].inds[1]->get_clear_value<uint32_t>();
        vector<uint32_t> inds = {alpha, other};
        exp_table1_val_.push_back({k, d, 0, inds});
#ifdef DEBUG
        cout << "exp_table1 " << k << " " << d << " " << alpha << " " << other << endl;
#endif
    }
    for (int i = 0; i < size_c_; i++) {
		int j = i + size_s_;
		uint32_t k = aug_table_[j].k->get_clear_value<uint32_t>();
        uint32_t d = aug_table_[j].d->get_clear_value<uint32_t>();
        uint32_t alpha = aug_table_[j].inds[1]->get_clear_value<uint32_t>();
        uint32_t other = aug_table_[j].inds[2]->get_clear_value<uint32_t>();
        vector<uint32_t> inds = {alpha, other};
        exp_table2_val_.push_back({k, d, 0, inds});
#ifdef DEBUG
        cout << "exp_table2 " << k << " " << d << " " << alpha << " " << other << endl;
#endif
	}
}

share* OblivJoin::prepare_exp_table(vector<Tuple>& src, vector<SharedTuple>& dst) {
    party_->reset();
    auto s_zero = circ_->PutCONSGate(ZERO, 32);
    auto s_one = circ_->PutCONSGate(ONE, 32);
    auto s_nil = circ_->PutCONSGate(NIL, 32);
    auto s_limit = s_one;
    for (int i = 0; i < src.size(); i++) {

#ifdef DEBUG
        auto k = circ_->PutINGate(src[i].k, 32, SERVER);
		auto d = circ_->PutINGate(src[i].d, 32, SERVER);
		auto g = circ_->PutINGate(src[i].inds[0], 32, SERVER);
		auto other = circ_->PutINGate(src[i].inds[1], 32, SERVER);
#else
        auto k = circ_->PutSharedINGate(src[i].k, 32);
		auto d = circ_->PutSharedINGate(src[i].d, 32);
		auto g = circ_->PutSharedINGate(src[i].inds[0], 32);
		auto other = circ_->PutSharedINGate(src[i].inds[1], 32);
#endif
        
		auto eq_zero = circ_->PutEQGate(g, s_zero);
		k = circ_->PutMUXGate(s_nil, k, eq_zero);
		d = circ_->PutMUXGate(s_nil, d, eq_zero);
		other = circ_->PutMUXGate(s_nil, other, eq_zero);
		auto f = circ_->PutMUXGate(s_nil, s_limit, eq_zero);
		s_limit = circ_->PutADDGate(s_limit, g);
        vector<share*> inds = {other, g, f};
        dst.push_back(SharedTuple{k, d, inds});
    }
    while (dst.size() < size_res_) {
        vector<share*> inds = {s_nil, s_nil, s_nil};
        dst.push_back(SharedTuple{s_nil, s_nil, inds});
    }
    return s_limit;
}

void OblivJoin::distribute(vector<SharedTuple>& table) {
    int j = prev_pow_two(size_res_);
    while (j >= 1) {
		for (int i = size_res_ - j - 1; i >= 0; i--) {
			uint32_t tmp = i + j;
			auto s_tmp = circ_->PutINGate(tmp, 32, SERVER);
            auto cond = circ_->PutGTGate(table[i].inds[2], s_tmp);
            auto t_i = table[i];
            table[i].cond_update(table[i + j], cond, circ_);
            table[i + j].cond_update(t_i, cond, circ_);
		}
		j = j / 2;
	}
}

void OblivJoin::fill_blank(vector<SharedTuple>& table, share* s_limit) {
    auto s_one = circ_->PutCONSGate(ONE, 32);
    auto s_nil = circ_->PutCONSGate(NIL, 32);
    auto cur = table[0];
    auto prev = table[0];
    for (uint32_t i = 1; i < size_res_; i++) {
		auto s_i = circ_->PutINGate(i + 2, 32, SERVER);
		auto valid = circ_->PutGTGate(s_i, s_limit);
		valid = circ_->PutXORGate(valid, s_one);
		cur = table[i];
		auto cond = circ_->PutEQGate(cur.k, s_nil);
        auto cond_not = circ_->PutXORGate(cond, s_one);

        cur.cond_update(prev, cond, circ_);
        prev.cond_update(cur, cond_not, circ_);
        table[i].cond_update(cur, cond, circ_);
	}
}

void OblivJoin::obliv_expand(bool pkfk) {
    cout << "obliv_expand" << endl;
    auto exp1_limit = prepare_exp_table(exp_table1_val_, exp_table1_);
#ifndef PARALLEL
    bitonic_sort<SharedTuple, exp_table_cmp>(exp_table1_, true, 0, size_res_, circ_);
#else
    exp1_limit = circ_->PutSharedOUTGate(exp1_limit);
    for (int i = 0; i < size_res_; i++) {
        exp_table1_[i].reveal(circ_);
    }
    party_->exec();
    vector<Tuple> exp_table1_v;
    for (int i = 0; i < size_res_; i++) {
        auto t = exp_table1_[i].get_value(circ_);
        exp_table1_v.push_back(t);
    }
    Tuple nv = Tuple::null_value();
    if (party_->get_role() == SERVER) {
        nv.k = 0;
        nv.inds[2] = 0x7fffffff;
    }
    auto exp1_limit_v = exp1_limit->get_clear_value<uint32_t>();

    BitonicSort bs1(party_, exp_table1_v, nv);
    bs1.sort(par_exp_table_cmp_tmp);
    exp_table1_v = bs1.get_output();

    exp_table1_.clear();
    for (int i = 0; i < size_res_; i++) {
        exp_table1_.push_back(SharedTuple{exp_table1_v[i], circ_, false});
    }
    exp1_limit = circ_->PutSharedINGate(exp1_limit_v, 32);
#endif
    distribute(exp_table1_);
    fill_blank(exp_table1_, exp1_limit);

#ifdef DEBUG
    for (int i = 0; i < size_res_; i++) {
        exp_table1_[i].reveal(circ_);
    }
#else
    if (SHARED) {
        for (int i = 0; i < size_res_; i++) {
            exp_table1_[i].reveal_share(circ_);
        }
    }
    else {
        for (int i = 0; i < size_res_; i++) {
            exp_table1_[i].reveal(circ_);
        }
    }
#endif

    party_->exec();
    for (int i = 0; i < size_res_; i++) {
        align_table1_val_.push_back(exp_table1_[i].get_value(circ_));
    }

    auto exp2_limit = prepare_exp_table(exp_table2_val_, exp_table2_);
#ifndef PARALLEL
    bitonic_sort<SharedTuple, exp_table_cmp>(exp_table2_, true, 0, size_res_, circ_);
#else
    exp2_limit = circ_->PutSharedOUTGate(exp2_limit);
    for (int i = 0; i < size_res_; i++) {
        exp_table2_[i].reveal(circ_);
    }
    party_->exec();
    vector<Tuple> exp_table2_v;
    for (int i = 0; i < size_res_; i++) {
        auto t = exp_table2_[i].get_value(circ_);
        exp_table2_v.push_back(t);
    }
    auto exp2_limit_v = exp2_limit->get_clear_value<uint32_t>();

    BitonicSort bs2(party_, exp_table2_v, nv);
    bs2.sort(par_exp_table_cmp_tmp);
    exp_table2_v = bs2.get_output();

    exp_table2_.clear();
    for (int i = 0; i < size_res_; i++) {
        exp_table2_.push_back(SharedTuple{exp_table2_v[i], circ_, false});
    }

    exp2_limit = circ_->PutSharedINGate(exp2_limit_v, 32);
#endif

    distribute(exp_table2_);
    fill_blank(exp_table2_, exp2_limit);

    auto s_nil = circ_->PutCONSGate(NIL, 32);

#ifdef DEBUG
    for (int i = 0; i < size_res_; i++) {
        exp_table2_[i].reveal(circ_);
    }
#else
    if (pkfk && !SHARED) {
        for (int i = 0; i < size_res_; i++) {
            exp_table2_[i].reveal(circ_);
        }
    }
    else {
        for (int i = 0; i < size_res_; i++) {
            exp_table2_[i].reveal_share(circ_);
        }
    }
#endif

    party_->exec();
    for (int i = 0; i < size_res_; i++) {
        align_table2_val_.push_back(exp_table2_[i].get_value(circ_));
    }

#ifdef DEBUG
    for (int i = 0; i < size_res_; i++) {
        cout << "exp1 " << align_table1_val_[i].k << " " << align_table1_val_[i].d << endl;
    }
    for (int i = 0; i < size_res_; i++) {
        cout << "exp2 " << align_table2_val_[i].k << " " << align_table2_val_[i].d << " " 
            << align_table2_val_[i].inds[0] << " " << align_table2_val_[i].inds[1] << endl;
    }
#endif
}

void OblivJoin::prepare_align_table() {
    party_->reset();
    auto s_one = circ_->PutCONSGate(ONE, 32);
    auto s_zero = circ_->PutCONSGate(ZERO, 32);
    auto s_nil = circ_->PutCONSGate(NIL, 32);
    exp_table2_.clear();
    for (int i = 0; i < size_res_; i++) {
#ifdef DEBUG
        auto s_k = circ_->PutINGate(align_table2_val_[i].k, 32, SERVER);
		auto s_d = circ_->PutINGate(align_table2_val_[i].d, 32, SERVER);
        auto s_alpha = circ_->PutINGate(align_table2_val_[i].inds[0], 32, SERVER);
        auto s_other = circ_->PutINGate(align_table2_val_[i].inds[1], 32, SERVER);
#else
		auto s_k = circ_->PutSharedINGate(align_table2_val_[i].k, 32);
		auto s_d = circ_->PutSharedINGate(align_table2_val_[i].d, 32);
        auto s_alpha = circ_->PutSharedINGate(align_table2_val_[i].inds[0], 32);
        auto s_other = circ_->PutSharedINGate(align_table2_val_[i].inds[1], 32);
#endif
        vector<share*> s_inds = {s_alpha, s_other};
		exp_table2_.push_back(SharedTuple{s_k, s_d, s_inds});
	}
    auto q = s_zero;
	auto prev_k = exp_table2_[0].k;
	auto div = s_zero;
	auto mod = s_one;
    vector<share*> s_inds = {s_zero, s_zero, s_zero};
    align_table_.push_back(SharedTuple{exp_table2_[0].k, exp_table2_[0].d, s_inds});
	for (int i = 1; i < size_res_; i++) {
		auto new_k = exp_table2_[i].k;
		auto k_eq = circ_->PutEQGate(prev_k, new_k);
		auto update = circ_->PutXORGate(k_eq, s_one);
		prev_k = new_k;

		div = circ_->PutMUXGate(s_zero, div, update);
		mod = circ_->PutMUXGate(s_zero, mod, update);
		auto eq_alpha = circ_->PutEQGate(exp_table2_[i].inds[1], mod);
        auto div_add_one = circ_->PutADDGate(div, s_one);
		div = circ_->PutMUXGate(div_add_one, div, eq_alpha);
		mod = circ_->PutMUXGate(s_zero, mod, eq_alpha);

        auto s_tmp = circ_->PutMULGate(mod, exp_table2_[i].inds[0]);
		auto s_ii = circ_->PutADDGate(div, s_tmp);
        s_inds = {s_ii, div, mod};
		align_table_.push_back(SharedTuple{exp_table2_[i].k, exp_table2_[i].d, s_inds});
		mod = circ_->PutADDGate(mod, s_one);
	}
#ifdef DEBUG
    for (int i = 0; i < size_res_; i++) {
        align_table_[i].reveal(circ_);
    }
    party_->exec();
    vector<Tuple> helper;
    for (int i = 0; i < size_res_; i++) {
        auto t = align_table_[i].get_value(circ_);
        cout << "align " << t.k << " " << t.d << " " << t.inds[0] << " " << t.inds[1] << " " << t.inds[2] << endl;
        helper.push_back(t);
    }
    party_->reset();
    align_table_.clear();
    for (int i = 0; i < size_res_; i++) {
        align_table_.push_back(SharedTuple{helper[i], circ_});
    }
#endif
}

void OblivJoin::align() {
    cout << "align" << endl;
    prepare_align_table();

#ifndef PARALLEL
    bitonic_sort<SharedTuple, align_table_cmp>(align_table_, true, 0, size_res_, circ_);
#else
    for (int i = 0; i < size_res_; i++) {
        align_table_[i].reveal_share(circ_);
    }
    party_->exec();
    vector<Tuple> align_table_v;
    for (int i = 0; i < size_res_; i++) {
        auto t = align_table_[i].get_value(circ_);
        align_table_v.push_back(t);
    }
    Tuple nv = Tuple::null_value();
    if (party_->get_role() == SERVER) {
        nv.k = 0x7fffffff;
    }
    BitonicSort bs(party_, align_table_v, nv);
    bs.sort(par_align_table_cmp);
    align_table_v = bs.get_output();
    align_table_.clear();
    for (int i = 0; i < size_res_; i++) {
        align_table_.push_back(SharedTuple{align_table_v[i], circ_, true});
    }
#endif

    if (SHARED) {
        for (int i = 0; i < size_res_; i++) {
            align_table_[i].reveal_share(circ_);
        }
    }
    else {
        for (int i = 0; i < size_res_; i++) {
            align_table_[i].reveal(circ_);
        }
    }
    party_->exec();
    align_table2_val_.clear();
    for (int i = 0; i < size_res_; i++) {
        align_table2_val_.push_back({align_table_[i].k->get_clear_value<uint32_t>(), align_table_[i].d->get_clear_value<uint32_t>()});
    }
}

void OblivJoin::join() {
    cout << "===== Oblivious Join =====" << endl;
    prepare_aug_table();
    split_table();
    obliv_expand(false);
    align();
    for (int i = 0; i < size_res_; i++) {
        auto k = align_table1_val_[i].k;
        auto d1 = align_table1_val_[i].d;
        auto d2 = align_table2_val_[i].d;
        res_table_.push_back(Tuple{k, d1, d2});
        cout << "(" << i + 1 << ") " << k << " " << d1 << " " << d2 << endl;
    }
    cout << "obliv comm: " << party_->get_comm() << endl;
    party_->clear_comm();
}

void OblivJoin::join_PKFK() {
    cout << "===== Oblivious Join (PK-FK) =====" << endl;
    prepare_aug_table();
    split_table();
    obliv_expand(true);
    for (int i = 0; i < size_res_; i++) {
        auto k = align_table1_val_[i].k;
        auto d1 = align_table1_val_[i].d;
        auto d2 = align_table2_val_[i].d;
        res_table_.push_back(Tuple{k, d1, d2});
        cout << "(" << i + 1 << ") " << k << " " << d1 << " " << d2 << endl;
    }
    cout << "obliv comm: " << party_->get_comm() << endl;
    party_->clear_comm();
}
#include "fast_join.h"
#include "cstdlib"
#include "obliv_permutation.h"

void FastJoin::join() {
    cout << "===== Fast Join =====" << endl;
    obliv_switch();
    party_->reset();
    circ_ = party_->get_circuit(S_BOOL);
    vector<uint32_t> zeros(size_c_);

#ifdef DEBUG
    auto server_k0 = circ_->PutSIMDINGate(size_c_, server_k0_.data(), 32, SERVER);
    auto server_k1 = circ_->PutSIMDINGate(size_c_, server_k1_.data(), 32, SERVER);
    auto server_d0 = circ_->PutSIMDINGate(size_c_, server_d0_.data(), 32, SERVER);
    auto server_d1 = circ_->PutSIMDINGate(size_c_, server_d1_.data(), 32, SERVER);
#else
    auto server_k0 = circ_->PutSharedSIMDINGate(size_c_, server_k0_.data(), 32);
    auto server_k1 = circ_->PutSharedSIMDINGate(size_c_, server_k1_.data(), 32);
    auto server_d0 = circ_->PutSharedSIMDINGate(size_c_, server_d0_.data(), 32);
    auto server_d1 = circ_->PutSharedSIMDINGate(size_c_, server_d1_.data(), 32);
#endif

    auto client_k = circ_->PutSIMDINGate(size_c_, client_k_.data(), 32, CLIENT);
    auto client_d = circ_->PutSIMDINGate(size_c_, client_d_.data(), 32, CLIENT);

    auto res_k = circ_->PutSIMDINGate(size_c_, zeros.data(), 32, SERVER);
    auto res_d1 = circ_->PutSIMDINGate(size_c_, zeros.data(), 32, SERVER);
    auto res_d2 = circ_->PutSIMDINGate(size_c_, zeros.data(), 32, SERVER);

    auto eq_k0 = circ_->PutEQGate(client_k, server_k0);
    auto eq_k1 = circ_->PutEQGate(client_k, server_k1);
    auto eq = circ_->PutXORGate(eq_k0, eq_k1);

    res_k = circ_->PutMUXGate(client_k, res_k, eq);
	res_d2 = circ_->PutMUXGate(client_d, res_d2, eq);
	res_d1 = circ_->PutMUXGate(server_d0, res_d1, eq_k0);
	res_d1 = circ_->PutMUXGate(server_d1, res_d1, eq_k1);

    if (SHARED) {
        res_k = circ_->PutSharedOUTGate(res_k);
	    res_d1 = circ_->PutSharedOUTGate(res_d1);
	    res_d2 = circ_->PutSharedOUTGate(res_d2);
    }
    else {
        res_k = circ_->PutOUTGate(res_k, ALL);
	    res_d1 = circ_->PutOUTGate(res_d1, ALL);
	    res_d2 = circ_->PutOUTGate(res_d2, ALL);
    }
	party_->exec();

    uint32_t *out_k, *out_d1, *out_d2;
	uint32_t nvals, bitlen;
    res_k->get_clear_value_vec(&out_k, &bitlen, &nvals);
	res_d1->get_clear_value_vec(&out_d1, &bitlen, &nvals);
	res_d2->get_clear_value_vec(&out_d2, &bitlen, &nvals);
    party_->reset();
    int cnt = 1;
	for (int i = 0; i < size_c_; i++) {
        if (out_k[i] == 0) {
            continue;
        }
		cout << "(" << cnt << ") " << out_k[i] << " " << out_d1[i] << " " << out_d2[i] << endl;
        cnt++;
	}
    cout << "fast comm: " << party_->get_comm() << endl;
    party_->clear_comm();
}

void FastJoin::cuckoo_hash(uint32_t key, uint32_t data) {
    int thres = 3 * size_s_;
    while (true) {
        vector<uint32_t> index = {single_hash(key, 0) % bin_num_, single_hash(key, 1) % bin_num_};
        if (cuckoo_key_[index[0]] == 0) {
            cuckoo_key_[index[0]] = key;
            cuckoo_data_[index[0]] = data;
            break;
        }
        if (cuckoo_key_[index[1]] == 0) {
            cuckoo_key_[index[1]] = key;
            cuckoo_data_[index[1]] = data;
            break;
        }
        auto hash_id = rand() % 2;
        auto replace = index[hash_id];
        cuckoo_key_[replace] ^= key ^= cuckoo_key_[replace] ^= key;
        cuckoo_data_[replace] ^= data ^= cuckoo_data_[replace] ^= data;
        if (thres-- < 0) {
            cout << "cuckoo hash exceed threshold!" << endl;
            exit(0);
        }
    }
}

void FastJoin::obliv_switch() {
    uint32_t bitlen, nvals;
    vector<uint32_t> k0, k1, d0, d1;
    vector<uint32_t> zeros(bin_num_);

    OblivPermutation op(party_->get_abyparty(), party_->get_role());
    if (party_->get_role() == CLIENT) {
        k0 = op.PermutorExtendedPermute(permute_table0_, zeros);
        k1 = op.PermutorExtendedPermute(permute_table1_, zeros);
        d0 = op.PermutorExtendedPermute(permute_table0_, zeros);
        d1 = op.PermutorExtendedPermute(permute_table1_, zeros);
    }
    else {
        k0 = op.SenderExtendedPermute(cuckoo_key_, size_c_);
        k1 = op.SenderExtendedPermute(cuckoo_key_, size_c_);
        d0 = op.SenderExtendedPermute(cuckoo_data_, size_c_);
        d1 = op.SenderExtendedPermute(cuckoo_data_, size_c_);
    }
    party_->reset();
    auto ac = party_->get_circuit(S_ARITH);
	auto bc = party_->get_circuit(S_BOOL);
    auto yc = party_->get_circuit(S_YAO);
    
    auto s_k0 = ac->PutSharedSIMDINGate(size_c_, k0.data(), 32);
    auto s_k1 = ac->PutSharedSIMDINGate(size_c_, k1.data(), 32);
    auto s_d0 = ac->PutSharedSIMDINGate(size_c_, d0.data(), 32);
    auto s_d1 = ac->PutSharedSIMDINGate(size_c_, d1.data(), 32);
    s_k0 = bc->PutA2BGate(s_k0, yc);
    s_k1 = bc->PutA2BGate(s_k1, yc);
    s_d0 = bc->PutA2BGate(s_d0, yc);
    s_d1 = bc->PutA2BGate(s_d1, yc);
#ifdef DEBUG
    s_k0 = bc->PutOUTGate(s_k0, ALL);
    s_k1 = bc->PutOUTGate(s_k1, ALL);
    s_d0 = bc->PutOUTGate(s_d0, ALL);
    s_d1 = bc->PutOUTGate(s_d1, ALL);
#else
	s_k0 = bc->PutSharedOUTGate(s_k0);
    s_k1 = bc->PutSharedOUTGate(s_k1);
    s_d0 = bc->PutSharedOUTGate(s_d0);
    s_d1 = bc->PutSharedOUTGate(s_d1);
#endif
	party_->exec();
    uint32_t *k0_array, *k1_array, *d0_array, *d1_array;
    s_k0->get_clear_value_vec(&k0_array, &bitlen, &nvals);
	s_k1->get_clear_value_vec(&k1_array, &bitlen, &nvals);
    s_d0->get_clear_value_vec(&d0_array, &bitlen, &nvals);
    s_d1->get_clear_value_vec(&d1_array, &bitlen, &nvals);

    for (int i = 0; i < nvals; i++) {
        server_k0_.push_back(k0_array[i]);
        server_k1_.push_back(k1_array[i]);
        server_d0_.push_back(d0_array[i]);
        server_d1_.push_back(d1_array[i]);
    }

#ifdef DEBUG
    if (party_->get_role() == CLIENT) {
        for (int i = 0; i < nvals; i++) {
            cout << "switch" << i << " " << client_k_[i] << " " << server_k0_[i] << " " << server_k1_[i] << endl;
        }
    }
#endif
	party_->reset();
}
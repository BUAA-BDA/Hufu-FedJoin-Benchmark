#pragma once
#include "ENCRYPTO_utils/crypto/crypto.h"
#include "ENCRYPTO_utils/parse_options.h"
#include "sharing/sharing.h"
#include "aby/abyparty.h"
#include "circuit/circuit.h"
#include "circuit/booleancircuits.h"
#include "circuit/arithmeticcircuits.h"
#include "murmur_hash3.h"
#include "obliv_permutation.h"
#include <vector>
#include <string>
#include <fstream>

#define SHARED 0
#define DEBUG 0

using namespace std;

class Tuple {
public:
    uint32_t k;
    uint32_t d;
    uint32_t d_other;
    vector<uint32_t> inds;
    Tuple(uint32_t kk, uint32_t dd): k(kk), d(dd), inds({0}) {}
    Tuple(uint32_t kk, uint32_t d1, uint32_t d2): k(kk), d(d1), d_other(d2), inds({0}) {}
    Tuple(uint32_t kk, uint32_t d1, uint32_t d2, uint32_t ind):
        k(kk), d(d1), d_other(d2), inds({ind}) {}
    Tuple(uint32_t kk, uint32_t d1, vector<uint32_t>& inds):
        k(kk), d(d1), inds(inds) {}
    Tuple(uint32_t kk, uint32_t d1, uint32_t d2, vector<uint32_t>& inds):
        k(kk), d(d1), d_other(d2), inds(inds) {}
    static Tuple null_value() {
        vector<uint32_t> inds = {0, 0, 0};
		return Tuple{0, 0, inds};
    }
    bool operator <(const Tuple& o) const {
        if (k == o.k) {
            return d < o.d;
        }
        return k < o.k;
    }
};

class SharedTuple {
public:
    share* k;
    share* d;
    vector<share*> inds;
    SharedTuple(share* kk, share* dd): k(kk), d(dd) {}

    SharedTuple(share* kk, share* dd, vector<share*>& ii):
        k(kk), d(dd), inds(ii) {}

    SharedTuple(Tuple& t, Circuit* circ, bool shared=false) {
        if (shared) {
            from_share(t, circ);
        }
        else {
            from_plain(t, circ);
        }
    }

    void cond_update(SharedTuple& st, share* cond, Circuit* circ) {
        k = circ->PutMUXGate(st.k, k, cond);
        d = circ->PutMUXGate(st.d, d, cond);
        for (int i = 0; i < inds.size(); i++) {
            inds[i] = circ->PutMUXGate(st.inds[i], inds[i], cond);
        }
    }

    void reveal(Circuit* circ) {
        k = circ->PutOUTGate(k, ALL);
        d = circ->PutOUTGate(d, ALL);
        for (int i = 0; i < inds.size(); i++) {
            inds[i] = circ->PutOUTGate(inds[i], ALL);
        }
    }

    void reveal_share(Circuit* circ) {
        k = circ->PutSharedOUTGate(k);
        d = circ->PutSharedOUTGate(d);
        for (int i = 0; i < inds.size(); i++) {
            inds[i] = circ->PutSharedOUTGate(inds[i]);
        }
    }

    void from_plain(Tuple& t, Circuit* circ) {
        k = circ->PutINGate(t.k, 32, SERVER);
        d = circ->PutINGate(t.d, 32, SERVER);
        inds.clear();
        for (int i = 0; i < t.inds.size(); i++) {
            inds.push_back(circ->PutINGate(t.inds[i], 32, SERVER));
        }
    }

    void from_share(Tuple& t, Circuit* circ) {
        k = circ->PutSharedINGate(t.k, 32);
        d = circ->PutSharedINGate(t.d, 32);
        inds.clear();
        for (int i = 0; i < t.inds.size(); i++) {
            inds.push_back(circ->PutSharedINGate(t.inds[i], 32));
        }
    }

    Tuple get_value(Circuit* circ) {
        auto kk = k->get_clear_value<uint32_t>();
        auto dd = d->get_clear_value<uint32_t>();
        vector<uint32_t> iind;
        for (int i = 0; i < inds.size(); i++) {
            iind.push_back(inds[i]->get_clear_value<uint32_t>());
        }
        return Tuple{kk, dd, iind};
    }

    static SharedTuple* null_value(Circuit* circ) {
        uint32_t NIL = -1;
		auto s_nil = circ->PutINGate(NIL, 32, SERVER);
        vector<share*> inds = {s_nil};
		return new SharedTuple{s_nil, s_nil, inds};
	}

    void print(Circuit* circ) {
        cout << k->get_clear_value<uint32_t>() << " ";
    }
};

class Party {
public:
    Party(e_role role, string addr, uint16_t port): role_(role) {
        abyparty_ = new ABYParty(role, addr, port);
    }

    Circuit* get_circuit(e_sharing type) {
        vector<Sharing*>& sharings = abyparty_->GetSharings();
        return sharings[type]->GetCircuitBuildRoutine();
    }

    ABYParty* get_abyparty() {
        return abyparty_;
    }

    e_role get_role() {
        return role_;
    }

    void exec() {
        abyparty_->ExecCircuit();
        auto abysetup = abyparty_->GetSentData(P_SETUP) + abyparty_->GetReceivedData(P_SETUP);
		auto abyonline = abyparty_->GetSentData(P_ONLINE) + abyparty_->GetReceivedData(P_ONLINE);
        comm_cost_ += (abysetup + abyonline);
    }

    void reset() {
        abyparty_->Reset();
    }

    void clear_comm() {
        comm_cost_ = 0;
    }

    double get_comm() {
        return comm_cost_ / (1024.0 * 1024.0);
    }

private:
    ABYParty* abyparty_;
    e_role role_;
    double comm_cost_;
};

void readRelation(string, vector<Tuple>&, int);

double wallClock();

inline int prev_pow_two(int x) {
    int y = 1;
    while (y < x) y <<= 1;
    return y >>= 1;
}

template <typename T, void (*cmp)(vector<T>&, bool, int, int, Circuit*)>
void bitonic_merge(vector<T>& table, bool asc, int lo, int hi, Circuit* circ) {
    if (hi <= lo + 1) {
        return;
    }
	int mid_len = prev_pow_two(hi - lo);
    for (int i = lo; i < hi - mid_len; i++) {
		cmp(table, asc, i, i + mid_len, circ);
	}
	bitonic_merge<T, cmp>(table, asc, lo, lo + mid_len, circ);
	bitonic_merge<T, cmp>(table, asc, lo + mid_len, hi, circ);
}

template <typename T, void (*cmp)(vector<T>&, bool, int, int, Circuit*)>
void bitonic_sort(vector<T>& table, bool asc, int lo, int hi, Circuit* circ) {
    int mid = lo + (hi - lo) / 2;
    if (mid == lo) {
        return;
    }
	bitonic_sort<T, cmp>(table, !asc, lo, mid, circ);
	bitonic_sort<T, cmp>(table, asc, mid, hi, circ);
	bitonic_merge<T, cmp>(table, asc, lo, hi, circ);
}

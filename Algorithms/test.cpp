#include "utils.h"
#include "obliv_permutation.h"
#include "obliv_stack.h"
#include "cstdlib"

ABYParty* party;
e_role role;
Circuit* circ;

void test_oep(int m, int n) {
    auto oep = OblivPermutation(party, role);
    vector<uint32_t> one(m, 1);
    vector<uint32_t> source(m);
    vector<uint32_t> dest(n);
    vector<uint32_t> out;
    for (int i = 0; i < m; i++) {
        source[i] = i;
    }
    for (int i = 0; i < n; i++) {
        dest[i] = rand() % m;
    }

    if (role == SERVER) {
        out = oep.PermutorExtendedPermute(dest, one);
    }
    else {
        out = oep.SenderExtendedPermute(source, n);
    }
    party->Reset();
    for (auto o : out) {
        cout << o << endl;
    }
    auto ac = party->GetSharings()[S_ARITH]->GetCircuitBuildRoutine();
    auto s_out = ac->PutSharedSIMDINGate(n, out.data(), 32);
    s_out = ac->PutOUTGate(s_out, ALL);
	party->ExecCircuit();
	uint32_t *res, nvals, bitlen;
	s_out->get_clear_value_vec(&res, &bitlen, &nvals);
    for (int i = 0; i < nvals; i++) {
        cout << res[i] << " " << dest[i] << endl;
    }

}

struct ShareWrap {
	share* s;
	ShareWrap(share* ss): s(ss) {}
    ShareWrap(uint32_t num) {
        s = circ->PutINGate(num, 32, SERVER);
    }
	void reveal(Circuit* circ) {
		s = circ->PutOUTGate(s, ALL);
	}
	void print() {
		cout << s->get_clear_value<uint32_t>() << " ";
	}
	void cond_update(ShareWrap& sw, share* cond, Circuit* circ) {
		s = circ->PutMUXGate(sw.s, s, cond);
	}
	static ShareWrap* null_value(Circuit* circ) {
		auto s_zero = circ->PutINGate((uint32_t)0, 32, SERVER);
		return new ShareWrap(s_zero);
	}
};

// void test_os(int k) {
//     OblivStack<ShareWrap> os(role, party, circ, k * 20);
//     uint32_t i = 1;
//     while (i < k) {
//         cout << i << endl;
//         bool cond = rand() % 2 == 0;
//         auto s_cond = circ->PutINGate((uint32_t)cond, 32, SERVER);
//         os.push(s_cond, ShareWrap{i * 3});
//         if (cond) {
//             i++;
//         }
//     }
//     i = 1;
//     vector<ShareWrap> res;
//     while (i < 9) {
//         bool cond = true;
//         auto s_cond = circ->PutINGate((uint32_t)cond, 32, SERVER);
//         auto tmp = os.pop(s_cond);
//         tmp.reveal(circ);
//         res.push_back(tmp);
//         i++;
//     }
//     os.reveal_log();
//     os.print();
//     cout << "result" << endl;
//     for (auto sw : res) {
//         sw.print();
//         cout << endl;
//     }
//     os.print_log();
// }

int main(int argc, char** argv) {
    role = SERVER;
    if (argv[1][0] == 'C') {
		role = CLIENT;
	}
    // auto x = (element_t*)malloc(sizeof(element_t) * 10);
    // party = new ABYParty(role);
    // test_oep(10, 15);
    // party->Reset();
    // circ = party->GetSharings()[S_BOOL]->GetCircuitBuildRoutine();
    // test_os(10);
}
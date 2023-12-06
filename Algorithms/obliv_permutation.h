#pragma once
#include <vector>
#include <cstdint>
#include <random>
#include "utils.h"

using namespace std;

class OblivPermutation {
public:
	OblivPermutation(ABYParty* abyparty, e_role role): abyparty_(abyparty), role_(role), block_(200000) {
		stdrng_.seed(1);
	}
	vector<uint32_t> SenderExtendedPermute(vector<uint32_t> &values, uint32_t N);
	vector<uint32_t> PermutorExtendedPermute(vector<uint32_t> &indices, vector<uint32_t> &permutorValues);

	struct Label {
        uint32_t input1;
        uint32_t input2;
        uint32_t output1;
        uint32_t output2;
    };

    struct GateBlinder {
        uint32_t upper;
        uint32_t lower;
    };

private:
	ABYParty* abyparty_;
	e_role role_;
	mt19937 stdrng_;
	uint32_t block_;
	const uint8_t GATE_NUM_MAP[27] = {0, 0, 1, 3, 5, 8, 11, 14, 17, 21, 25, 29, 33, 37, 41, 45, 49, 54, 59, 64, 69, 74, 79, 84, 89, 94, 99};
	
	uint64_t pack(uint32_t a, uint32_t b) {
    	return (uint64_t)a << 32 | b;
	}

	void unpack(uint64_t c, uint32_t &a, uint32_t &b) {
    	a = c >> 32;
    	b = c & 0xffffffff;
	}

	int floor_log2(int a) {
   		int ret = -1;
   		while (a > 0) {
       		a >>= 1;
       		ret++;
   		}
   		return ret;
	}

	vector<uint64_t> OT(vector<uint64_t>& msg0, vector<uint64_t>& msg1, vector<uint32_t>& select, bool isSender);
	uint32_t ComputeGateNum(int n);
	void GenSelectionBits(const uint32_t *permuIndices, int size, bool *bits);
	void EvaluateGate(uint32_t &v0, uint32_t &v1, GateBlinder blinder, uint8_t bit);
	void EvaluateNetwork(uint32_t *values, int size, const bool *bits, const GateBlinder *blinders);
	void WriteGateLabels(uint32_t *inputLabel, int size, Label *gateLabels);
	vector<uint32_t> SenderPermute(vector<uint32_t> &values);
	vector<uint32_t> PermutorPermute(vector<uint32_t> &indices, vector<uint32_t> &permutorValues);
	vector<uint32_t> SenderReplicate(vector<uint32_t> &values);
	vector<uint32_t> PermutorReplicate(vector<uint32_t> &repBits, vector<uint32_t> &permutorValues);

};
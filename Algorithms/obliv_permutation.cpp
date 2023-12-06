#include "obliv_permutation.h"
#include <cassert>
#include <iostream>
#include <cstdint>

vector<uint64_t> OblivPermutation::OT(vector<uint64_t>& msg0, vector<uint64_t>& msg1, 
    vector<uint32_t>& select, bool isSender) {
	auto circ = abyparty_->GetSharings()[S_BOOL]->GetCircuitBuildRoutine();
    auto sender_role = isSender? role_: (e_role)(1 - role_);
    auto receiver_role = (e_role)(1 - sender_role);
    auto size = isSender? msg0.size() : select.size();
    uint32_t block_num = size / block_;
    
    uint32_t offset = size - block_num * block_;
    vector<uint64_t> ret;
    // cout << "OT " << size << " " << block_num << " " << offset << endl;
    
    for (int b = 0; b < block_num; b++) {
        abyparty_->Reset();
        share *s_msg0, *s_msg1, *s_select;
        if (isSender) {
            s_msg0 = circ->PutSIMDINGate(block_, msg0.data() + b * block_, 64, sender_role);
            s_msg1 = circ->PutSIMDINGate(block_, msg1.data() + b * block_, 64, sender_role);
            s_select = circ->PutDummySIMDINGate(block_, 32);
        }
        else {
            s_msg0 = circ->PutDummySIMDINGate(block_, 64);
            s_msg1 = circ->PutDummySIMDINGate(block_, 64);
            s_select = circ->PutSIMDINGate(block_, select.data() + b * block_, 32, receiver_role);
        }

        auto s_ret = circ->PutMUXGate(s_msg1, s_msg0, s_select);
        s_ret = circ->PutOUTGate(s_ret, ALL);
        abyparty_->ExecCircuit();

        if (!isSender) {
            uint64_t *ret_val;
            uint32_t nvals, bitlen;
            s_ret->get_clear_value_vec(&ret_val, &bitlen, &nvals);
            for (int i = 0; i < nvals; i++) {
                ret.push_back(ret_val[i]);
            }
        }
    }

    abyparty_->Reset();
    share *s_msg0, *s_msg1, *s_select;
    if (isSender) {
        s_msg0 = circ->PutSIMDINGate(offset, msg0.data() + block_num * block_, 64, sender_role);
        s_msg1 = circ->PutSIMDINGate(offset, msg1.data() + block_num * block_, 64, sender_role);
        s_select = circ->PutDummySIMDINGate(offset, 32);
    }
    else {
        s_msg0 = circ->PutDummySIMDINGate(offset, 64);
        s_msg1 = circ->PutDummySIMDINGate(offset, 64);
        s_select = circ->PutSIMDINGate(offset, select.data() + block_num * block_, 32, receiver_role);
    }

    auto s_ret = circ->PutMUXGate(s_msg1, s_msg0, s_select);
    s_ret = circ->PutOUTGate(s_ret, ALL);
    abyparty_->ExecCircuit();

    if (!isSender) {
        uint64_t *ret_val;
        uint32_t nvals, bitlen;
        s_ret->get_clear_value_vec(&ret_val, &bitlen, &nvals);
        for (int i = 0; i < nvals; i++) {
            ret.push_back(ret_val[i]);
        }
        return ret;
    }

    vector<uint64_t> nil(size);
    return nil;
}

uint32_t OblivPermutation::ComputeGateNum(int n) {
    if (n < sizeof(GATE_NUM_MAP) / sizeof(uint8_t)) {
        return GATE_NUM_MAP[n];
    }
 	int power = floor_log2(n) + 1;
    return power * n + 1 - (1 << power);
}

void OblivPermutation::GenSelectionBits(const uint32_t *permuIndices, int size, bool *bits) {
    if (size == 2) {
        bits[0] = permuIndices[0];
    }
    if (size <= 2) {
        return;
    }
    uint32_t *invPermuIndices = new uint32_t[size];
    for (int i = 0; i < size; i++) {
        invPermuIndices[permuIndices[i]] = i;
    }
    bool odd = size & 1;

    // Solve the edge coloring problem
    // flag=0: non-specified; flag=1: upperNetwork; flag=2: lowerNetwork
    char *leftFlag = new char[size]();
    char *rightFlag = new char[size]();
    int rightPointer = size - 1;
    int leftPointer;
    while (rightFlag[rightPointer] == 0) {
        rightFlag[rightPointer] = 2;
        leftPointer = permuIndices[rightPointer];
        leftFlag[leftPointer] = 2;
        if (odd && leftPointer == size - 1) {
            break;
        }
        leftPointer = leftPointer & 1 ? leftPointer - 1 : leftPointer + 1;
        leftFlag[leftPointer] = 1;
        rightPointer = invPermuIndices[leftPointer];
        rightFlag[rightPointer] = 1;
        rightPointer = rightPointer & 1 ? rightPointer - 1 : rightPointer + 1;
    }
    for (int i = 0; i < size - 1; i++) {
        rightPointer = i;
        while (rightFlag[rightPointer] == 0) {
            rightFlag[rightPointer] = 2;
            leftPointer = permuIndices[rightPointer];
            leftFlag[leftPointer] = 2;
            leftPointer = leftPointer & 1 ? leftPointer - 1 : leftPointer + 1;
            leftFlag[leftPointer] = 1;
            rightPointer = invPermuIndices[leftPointer];
            rightFlag[rightPointer] = 1;
            rightPointer = rightPointer & 1 ? rightPointer - 1 : rightPointer + 1;
        }
    }
    delete[] invPermuIndices;

    // Determine bits on left gates
    int halfSize = size / 2;
    for (int i = 0; i < halfSize; i++) {
        bits[i] = leftFlag[2 * i] == 2;
    }
    int upperIndex = halfSize;
    int uppergateNum = ComputeGateNum(halfSize);
    int lowerIndex = upperIndex + uppergateNum;
    int rightGateIndex = lowerIndex + (odd ? ComputeGateNum(halfSize + 1) : uppergateNum);
    // Determine bits on right gates
    for (int i = 0; i < halfSize - 1; i++)
        bits[rightGateIndex + i] = rightFlag[2 * i] == 2;
    if (odd)
        bits[rightGateIndex + halfSize - 1] = rightFlag[size - 2] == 1;

    delete[] leftFlag;
    delete[] rightFlag;

    // Compute upper network
    uint32_t *upperIndices = new uint32_t[halfSize];
    for (int i = 0; i < halfSize - 1 + odd; i++)
        upperIndices[i] = permuIndices[2 * i + bits[rightGateIndex + i]] / 2;
    if (!odd) {
        upperIndices[halfSize - 1] = permuIndices[size - 2] / 2;
    }
    GenSelectionBits(upperIndices, halfSize, bits + upperIndex);
    delete[] upperIndices;

    // Compute lower network
    int lowerSize = halfSize + odd;
    uint32_t *lowerIndices = new uint32_t[lowerSize];
    for (int i = 0; i < halfSize - 1 + odd; i++) {
        lowerIndices[i] = permuIndices[2 * i + 1 - bits[rightGateIndex + i]] / 2;
    }
    if (odd) {
        lowerIndices[halfSize] = permuIndices[size - 1] / 2;
    }
        
    else {
        lowerIndices[halfSize - 1] = permuIndices[2 * halfSize - 1] / 2;
    }
    GenSelectionBits(lowerIndices, lowerSize, bits + lowerIndex);
    delete[] lowerIndices;
}

// Inputs of the gate: v0=x0-r1, v1=x1-r2
// Outputs of the gate: if bit==1 then v0=x1-r3, v1=x0-r4; otherwise v0=x0-r3, v1=x1-r4
// m0=r1-r3, m1=r2-r4
void OblivPermutation::EvaluateGate(uint32_t &v0, uint32_t &v1, GateBlinder blinder, uint8_t bit) {
    if (bit) {
        uint32_t temp = v1 + blinder.upper;
        v1 = v0 + blinder.lower;
        v0 = temp;
    }
    else {
        v0 += blinder.upper;
        v1 += blinder.lower;
    }
}

// If you want to apply the original exchange operation, set blinders to be 0;
void OblivPermutation::EvaluateNetwork(uint32_t *values, int size, const bool *bits, const GateBlinder *blinders) {
    if (size == 2) {
        EvaluateGate(values[0], values[1], blinders[0], bits[0]);
    }
    if (size <= 2) {
        return;
    }
    int odd = size & 1;
    int halfSize = size / 2;

    // Compute left gates
    for (int i = 0; i < halfSize; i++) {
        EvaluateGate(values[2 * i], values[2 * i + 1], blinders[i], bits[i]);
    }
    bits += halfSize;
    blinders += halfSize;

    // Compute upper subnetwork
    uint32_t *upperValues = new uint32_t[halfSize];
    for (int i = 0; i < halfSize; i++)
        upperValues[i] = values[i * 2];
    EvaluateNetwork(upperValues, halfSize, bits, blinders);
    int uppergateNum = ComputeGateNum(halfSize);
    bits += uppergateNum;
    blinders += uppergateNum;

    // Compute lower subnetwork
    int lowerSize = halfSize + odd;
    uint32_t *lowerValues = new uint32_t[lowerSize];
    for (int i = 0; i < halfSize; i++) {
        lowerValues[i] = values[i * 2 + 1];
    }
    if (odd) {
        lowerValues[lowerSize - 1] = values[size - 1];
    }
    EvaluateNetwork(lowerValues, lowerSize, bits, blinders);
    int lowergateNum = odd ? ComputeGateNum(lowerSize) : uppergateNum;
    bits += lowergateNum;
    blinders += lowergateNum;

    // Deal with outputs of subnetworks
    for (int i = 0; i < halfSize; i++) {
        values[2 * i] = upperValues[i];
        values[2 * i + 1] = lowerValues[i];
    }
    if (odd) {
        values[size - 1] = lowerValues[lowerSize - 1];
    }

    // Compute right gates
    for (int i = 0; i < halfSize - 1 + odd; i++) {
        EvaluateGate(values[2 * i], values[2 * i + 1], blinders[i], bits[i]);
    }

    delete[] upperValues;
    delete[] lowerValues;
}

void OblivPermutation::WriteGateLabels(uint32_t *inputLabel, int size, Label *gateLabels) {
    if (size == 2) {
        gateLabels[0].input1 = inputLabel[0];
        gateLabels[0].input2 = inputLabel[1];
        gateLabels[0].output1 = stdrng_();
        gateLabels[0].output2 = stdrng_();
        inputLabel[0] = gateLabels[0].output1;
        inputLabel[1] = gateLabels[0].output2;
    }

    if (size <= 2) {
        return;
    }
    int odd = size & 1;
    int halfSize = size / 2;

    // Compute left gates
    for (int i = 0; i < halfSize; i++) {
        gateLabels[i].input1 = inputLabel[2 * i];
        gateLabels[i].input2 = inputLabel[2 * i + 1];
        gateLabels[i].output1 = stdrng_();
        gateLabels[i].output2 = stdrng_();
        inputLabel[2 * i] = gateLabels[i].output1;
        inputLabel[2 * i + 1] = gateLabels[i].output2;
    }
    gateLabels += halfSize;

    // Compute upper subnetwork
    uint32_t *upperInputs = new uint32_t[halfSize];
    for (int i = 0; i < halfSize; i++)
        upperInputs[i] = inputLabel[2 * i];
    WriteGateLabels(upperInputs, halfSize, gateLabels);
    int uppergateNum = ComputeGateNum(halfSize);
    gateLabels += uppergateNum;

    // Compute lower subnetwork
    int lowerSize = halfSize + odd;
    uint32_t *lowerInputs = new uint32_t[lowerSize];
    for (int i = 0; i < halfSize; i++) {
        lowerInputs[i] = inputLabel[2 * i + 1];
    }
    if (odd) {
        lowerInputs[lowerSize - 1] = inputLabel[size - 1];
    }
    WriteGateLabels(lowerInputs, lowerSize, gateLabels);
    int lowergateNum = odd ? ComputeGateNum(lowerSize) : uppergateNum;
    gateLabels += lowergateNum;

    // Deal with outputs of subnetworks
    for (int i = 0; i < halfSize; i++) {
        inputLabel[2 * i] = upperInputs[i];
        inputLabel[2 * i + 1] = lowerInputs[i];
    }
    if (odd) // the last element
        inputLabel[size - 1] = lowerInputs[lowerSize - 1];

    // Compute right gates
    for (int i = 0; i < halfSize - 1 + odd; i++) {
        gateLabels[i].input1 = inputLabel[2 * i];
        gateLabels[i].input2 = inputLabel[2 * i + 1];
        gateLabels[i].output1 = stdrng_();
        gateLabels[i].output2 = stdrng_();
        inputLabel[2 * i] = gateLabels[i].output1;
        inputLabel[2 * i + 1] = gateLabels[i].output2;
    }
    delete[] upperInputs;
    delete[] lowerInputs;
}

vector<uint32_t> OblivPermutation::SenderPermute(vector<uint32_t> &values) {
    auto size = values.size();
    uint32_t gateNum = ComputeGateNum(size);
    // Sender generates blinded inputs
    Label *gateLabels = new Label[gateNum];

    vector<uint32_t> out = values;
    // Locally randomly writes labels for each gate
    WriteGateLabels(&out[0], size, gateLabels);
    vector<uint64_t> msg0(gateNum);
    vector<uint64_t> msg1(gateNum);

    for (uint32_t i = 0; i < gateNum; i++) {
        Label label = gateLabels[i];
        msg0[i] = pack(label.input1 - label.output1, label.input2 - label.output2);
        msg1[i] = pack(label.input2 - label.output1, label.input1 - label.output2);
    }

    vector<uint32_t> unused;
    OT(msg0, msg1, unused, true);

    delete[] gateLabels;
    return out;
}

vector<uint32_t> OblivPermutation::PermutorPermute(vector<uint32_t> &indices, vector<uint32_t> &permutorValues) {
    auto size = indices.size();
    vector<bool> flag(size, false);
    for (auto i : indices) {
        flag[i] = true;
    }
    for (auto f : flag) {
        assert(f && "Not a permutation!");
    }
    assert(size == permutorValues.size());
    uint32_t gateNum = ComputeGateNum(size);
    bool *selectBits_arr = new bool[gateNum];
    GenSelectionBits(indices.data(), size, selectBits_arr);
    vector<uint32_t> selectBits(selectBits_arr, selectBits_arr + gateNum);
    vector<uint64_t> msg;
    vector<uint64_t> unused;
    msg = OT(unused, unused, selectBits, false);

    GateBlinder *gateBlinders = new GateBlinder[gateNum];
    for (uint32_t i = 0; i < gateNum; i++) {
        unpack(msg[i], gateBlinders[i].upper, gateBlinders[i].lower);
    }
    vector<uint32_t> out = permutorValues;
    EvaluateNetwork(&out[0], size, selectBits_arr, gateBlinders);
    delete[] gateBlinders;
    delete[] selectBits_arr;
    return out;
}

vector<uint32_t> OblivPermutation::SenderReplicate(vector<uint32_t> &values) {
    auto size = values.size();
    Label *labels = new Label[size - 1];
    vector<uint32_t> out(size);
    for (uint32_t i = 0; i < size - 1; i++) {
        labels[i].input1 = i == 0 ? values[0] : labels[i - 1].output2;
        labels[i].input2 = values[i + 1];
        out[i] = labels[i].output1 = stdrng_();
        labels[i].output2 = stdrng_();
    }
    out[size - 1] = labels[size - 2].output2;
    vector<uint64_t> msg0(size - 1);
    vector<uint64_t> msg1(size - 1);
    for (uint32_t i = 0; i < size - 1; i++) {
        msg0[i] = pack(labels[i].input1 - labels[i].output1, labels[i].input2 - labels[i].output2);
        msg1[i] = pack(labels[i].input1 - labels[i].output1, labels[i].input1 - labels[i].output2);
    }
    vector<uint32_t> unused;
    OT(msg0, msg1, unused, true);
    delete[] labels;
    return out;
}

vector<uint32_t> OblivPermutation::PermutorReplicate(vector<uint32_t> &repBits, vector<uint32_t> &permutorValues) {
    auto size = repBits.size() + 1;
    assert(size == permutorValues.size());
    vector<uint64_t> msg;
    vector<uint64_t> unused;
    msg = OT(unused, unused, repBits, false);
    vector<uint32_t> out(size);
    Label *labels = new Label[size - 1];
    for (uint32_t i = 0; i < size - 1; i++) {
        uint32_t upper, lower;
        unpack(msg[i], upper, lower);
        labels[i].input1 = i == 0 ? permutorValues[i] : labels[i - 1].output2;
        labels[i].input2 = permutorValues[i + 1];
        out[i] = labels[i].output1 = labels[i].input1 + upper;
        labels[i].output2 = (repBits[i] ? labels[i].input1 : labels[i].input2) + lower;
    }
    out[size - 1] = labels[size - 2].output2;
    delete[] labels;
    return out;
}

vector<uint32_t> OblivPermutation::SenderExtendedPermute(vector<uint32_t> &values, uint32_t N) {
    auto M = values.size();
    M = N > M? N : M;
    auto extendedValues = values;
    extendedValues.resize(M, 0);
    auto out = SenderPermute(extendedValues);
    out.resize(N);
    out = SenderReplicate(out);
    return SenderPermute(out);
}

vector<uint32_t> OblivPermutation::PermutorExtendedPermute(vector<uint32_t> &indices, vector<uint32_t> &permutorValues) {
    auto M = permutorValues.size();
    auto N = indices.size();
    M = N > M? N : M;
    vector<uint32_t> indicesCount(M, 0);
    for (uint32_t i = 0; i < N; i++) {
        assert(indices[i] < permutorValues.size());
        indicesCount[indices[i]]++;
    }
    vector<uint32_t> firstPermu(M);
    uint32_t dummyIndex = 0, fPIndex = 0;
    vector<uint32_t> repBits(N - 1);

    // We call those index with indicesCount[index]==0 as dummy index
    for (uint32_t i = 0; i < M; i++) {
        if (indicesCount[i] > 0) {
            firstPermu[fPIndex++] = i;
            for (uint32_t j = 0; j < indicesCount[i] - 1; j++) {
                while (indicesCount[dummyIndex] > 0)
                    dummyIndex++;
                firstPermu[fPIndex++] = dummyIndex++;
            }
        }
    }
    while (fPIndex < M) {
        while (indicesCount[dummyIndex] > 0) {
            dummyIndex++;
        }
        firstPermu[fPIndex++] = dummyIndex++;
    }

    auto out = permutorValues;
    out.resize(M);
    out = PermutorPermute(firstPermu, out);
    out.resize(N);
    for (uint32_t i = 0; i < N - 1; i++) {
        repBits[i] = indicesCount[firstPermu[i + 1]] == 0;
    }
    out = PermutorReplicate(repBits, out);
    vector<uint32_t> pointers(M);
    uint32_t sum = 0;
    for (uint32_t i = 0; i < M; i++) {
        pointers[i] = sum;
        sum += indicesCount[i];
    }
    vector<uint32_t> totalMap(N);
    for (uint32_t i = 0; i < N; i++) {
        totalMap[i] = firstPermu[pointers[indices[i]]++];
    }
    vector<uint32_t> invFirstPermu(M);
    for (uint32_t i = 0; i < M; i++) {
        invFirstPermu[firstPermu[i]] = i;
    }
    vector<uint32_t> secondPermu(N);
    for (int i = 0; i < N; i++) {
        secondPermu[i] = invFirstPermu[totalMap[i]];
    }
    return PermutorPermute(secondPermu, out);
}

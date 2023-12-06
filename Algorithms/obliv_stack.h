#include <ENCRYPTO_utils/crypto/crypto.h>
#include <ENCRYPTO_utils/parse_options.h>
#include "sharing/sharing.h"
#include "aby/abyparty.h"
#include "circuit/circuit.h"
#include "circuit/booleancircuits.h"
#include "circuit/arithmeticcircuits.h"
#include "utils.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <ctime>
#include <cmath>
#include <string>
using namespace std;

template<typename T> 
class OblivStack {
public:
	OblivStack(e_role role, ABYParty* party, Circuit* circ, int total_op): 
		role_(role), party_(party), circ_(circ), total_op_(total_op), total_pop_(0), total_push_(0) {
		group_num_ = (int)log2((double)total_op / 5.0) + 1;
		int num = 5;
		for (int i = 1; i <= group_num_; i++) {
			push_cnt_.push_back(0);
			pop_cnt_.push_back(0);

			auto pos = circ_->PutINGate((uint32_t)-1, 32, SERVER);
			group_pos_.push_back(pos);
			vector<T> tmp;
			for (int i = 0; i <= num; i++) {
				tmp.push_back(*(T::null_value(circ_)));
			}
			groups_.push_back(tmp);
			num *= 2;
		}
	}

	T top(share* cond, string msg = "") {
		auto ret = groups_[0][0];
		auto s_one = circ_->PutINGate((uint32_t)1, 32, SERVER);
		auto s_zero = circ_->PutINGate((uint32_t)0, 32, SERVER);
		auto eof = circ_->PutGTGate(s_zero, group_pos_[0]);
		ret.cond_update(*(T::null_value(circ_)), eof, circ_);
		auto cond_neg = circ_->PutXORGate(cond, s_one);
		ret.cond_update(*(T::null_value(circ_)), cond_neg, circ_);

#ifdef DEBUG
		logs_type_.push_back("top");
		logs_cond_.push_back(cond);
		logs_ele_.push_back(ret);
		logs_msg_.push_back(msg);
#endif
		return ret;
	}

	T pop(share* cond, string msg = "") {
		total_pop_++;
		assert(total_pop_ < total_op_);
		auto s_one = circ_->PutINGate((uint32_t)1, 32, SERVER);
		auto s_zero = circ_->PutINGate((uint32_t)0, 32, SERVER);
		auto ret = groups_[0][0];
		auto eof = circ_->PutGTGate(s_zero, group_pos_[0]);
		ret.cond_update(*(T::null_value(circ_)), eof, circ_);
		auto cond_neg = circ_->PutXORGate(cond, s_one);
		ret.cond_update(*(T::null_value(circ_)), cond_neg, circ_);
		for (int i = 0; i < 4; i++) {
            groups_[0][i].cond_update(groups_[0][i + 1], cond, circ_);
		}
		auto pos_dec = circ_->PutSUBGate(group_pos_[0], s_one);
		group_pos_[0] = circ_->PutMUXGate(pos_dec, group_pos_[0], cond);
		pop_cnt_[0]++;
        uint32_t thres = 1;
		
		for (int i = 0; i < group_num_; i++) {
            if (pop_cnt_[i] == 2) {
                pop_cnt_[i] = 0;
                pop_cnt_[i + 1] += 1;
				auto s_thres = circ_->PutINGate(thres, 32, SERVER);
				auto s_neg = circ_->PutINGate((uint32_t)-1, 32, SERVER);
				auto pos0_eq_neg = circ_->PutEQGate(group_pos_[i], s_neg);
				auto pos1_eq_neg = circ_->PutEQGate(group_pos_[i + 1], s_neg);
				auto pos1_noteq_neg = circ_->PutXORGate(pos1_eq_neg, s_one);
                auto exceed = circ_->PutGTGate(s_thres, group_pos_[i]);
				exceed = circ_->PutADDGate(exceed, pos0_eq_neg);
				exceed = circ_->PutANDGate(exceed, pos1_noteq_neg);
				shift_left(exceed, i);
                thres *= 2;
            }
			else {
				break;
			}
        }
#ifdef DEBUG
		logs_type_.push_back("pop");
		logs_cond_.push_back(cond);
		logs_ele_.push_back(ret);
		logs_msg_.push_back(msg);
#endif
		return ret;
	}

	void push(share* cond, T ele, string msg = "") {
		total_push_++;
		assert(total_push_ < total_op_);
		auto s_one = circ_->PutCONSGate((uint32_t)1, 32);
		for (int i = 4; i > 0; i--) {
            groups_[0][i].cond_update(groups_[0][i - 1], cond, circ_);
        }
		auto pos_inc = circ_->PutADDGate(group_pos_[0], s_one);
		group_pos_[0] = circ_->PutMUXGate(pos_inc, group_pos_[0], cond);
        groups_[0][0].cond_update(ele, cond, circ_);
		push_cnt_[0]++;
        uint32_t thres = 4;
		
		for (int i = 0; i < group_num_; i++) {
			if (push_cnt_[i] == 2) {
				push_cnt_[i] = 0;
				push_cnt_[i + 1]++;
				auto s_thres = circ_->PutINGate(thres - 2, 32, SERVER);
				auto s_neg = circ_->PutINGate((uint32_t)-1, 32, SERVER);
				auto pos_eq_neg = circ_->PutEQGate(group_pos_[i], s_neg);
				auto pos_noteq_neg = circ_->PutXORGate(pos_eq_neg, s_one);
				auto exceed = circ_->PutGTGate(group_pos_[i], s_thres);
				exceed = circ_->PutANDGate(exceed, pos_noteq_neg);
				shift_right(exceed, i);
				thres *= 2;
			}
			else {
				break;
			}
		}
#ifdef DEBUG
		logs_type_.push_back("push");
		logs_cond_.push_back(cond);
		logs_ele_.push_back(ele);
		logs_msg_.push_back(msg);
#endif
	}

	void reveal_log() {
#ifdef DEBUG
		for (int i = 0; i < logs_type_.size(); i++) {
			logs_cond_[i] = circ_->PutOUTGate(logs_cond_[i], ALL);
			logs_ele_[i].reveal(circ_);
		}
#endif
	}

	void print_log() {
#ifdef DEBUG
		for (int i = 0; i < logs_type_.size(); i++) {
			cout << logs_type_[i] << " ";
			cout << logs_cond_[i]->get_clear_value<uint32_t>() << " ";
			logs_ele_[i].print(circ_);
			cout << logs_msg_[i] << endl;
		}
#endif
	}

	void print() {
		cout << "print obliv_stack" << endl;
		int num = 5;
		for (int i = 0; i < group_num_; i++) {
			group_pos_[i] = circ_->PutOUTGate(group_pos_[i], ALL);
			for (int j = 0; j < num; j++) {
				groups_[i][j].reveal(circ_);
			}
			num *= 2;
		}
		party_->ExecCircuit();
		num = 5;
		for (int i = 0; i < group_num_; i++) {
			cout << group_pos_[i]->get_clear_value<uint32_t>() << " | ";
			for (int j = 0; j < num; j++) {
				groups_[i][j].print();
            }
			cout << endl;
			num *= 2;
		}
	}

private:
	e_role role_;
	ABYParty* party_;
	Circuit* circ_;
	int total_op_;
	int group_num_;
	vector<vector<T>> groups_;
	vector<share*> group_pos_;
    vector<int> push_cnt_;
    vector<int> pop_cnt_;
	int total_pop_;
	int total_push_;

#ifdef DEBUG
	vector<string> logs_type_;
	vector<share*> logs_cond_;
	vector<T> logs_ele_;
	vector<string> logs_msg_;
#endif

	void shift_right(share* cond, int index) {
        uint32_t total = (int)pow(2, index + 1);
		auto s_zero = circ_->PutINGate((uint32_t)0, 32, SERVER);
		auto s_one = circ_->PutINGate((uint32_t)1, 32, SERVER);
		auto s_neg = circ_->PutINGate((uint32_t)-1, 32, SERVER);
		auto s_total = circ_->PutINGate(total, 32, SERVER);
		auto pos1 = group_pos_[index + 1];
		auto pos1_add_total = circ_->PutADDGate(pos1, s_total);
		auto pos1_eq_neg = circ_->PutEQGate(pos1, s_neg);
		auto pos1_noteq_neg = circ_->PutXORGate(pos1_eq_neg, s_one);
		for (uint32_t i = 5 * total - 1; i >= total; i--) {
			auto cur_cond = circ_->PutANDGate(cond, pos1_noteq_neg);
			auto s_i = circ_->PutINGate(i, 32, SERVER);
			auto tmp = circ_->PutGTGate(s_i, pos1_add_total);
			auto s_i_le_pos_add_total = circ_->PutXORGate(tmp, s_one);
			cur_cond = circ_->PutANDGate(cur_cond, s_i_le_pos_add_total);
            groups_[index + 1][i].cond_update(groups_[index + 1][i - total], cur_cond, circ_);
		}

		for (int j = 4; j <= 5; j++) {  // group_pos_[index] only has 2 possible value
			uint32_t start = j * total / 2 - 1;
			auto s_start = circ_->PutINGate(start, 32, SERVER);
			auto cur_cond = circ_->PutEQGate(group_pos_[index], s_start);
			cur_cond = circ_->PutANDGate(cond, cur_cond);

			for (uint32_t i = 0; i < total; i++) {
				int k = start - total + i + 1;
                groups_[index + 1][i].cond_update(groups_[index][k], cur_cond, circ_);
                groups_[index][k].cond_update(*(T::null_value(circ_)), cur_cond, circ_);
			}
		}
		group_pos_[index + 1] = circ_->PutMUXGate(pos1_add_total, group_pos_[index + 1], cond);
		auto pos0_sub_total = circ_->PutSUBGate(group_pos_[index], s_total);
		group_pos_[index] = circ_->PutMUXGate(pos0_sub_total, group_pos_[index], cond);
	}

	void shift_left(share* cond, int index) {
        uint32_t total = (int)pow(2, index + 1);
		auto s_total = circ_->PutINGate(total, 32, SERVER);
		auto pos0 = group_pos_[index];
		auto pos1 = group_pos_[index + 1];
		for (int j = 0; j <= 1; j++) {  // group_pos_[index] only has 2 possible value
			uint32_t start = j * total / 2 - 1;
			auto s_start = circ_->PutINGate(start, 32, SERVER);
			auto cur_cond = circ_->PutEQGate(pos0, s_start);
			cur_cond = circ_->PutANDGate(cur_cond, cond);
			for (int i = 0; i < total; i++) {
				int loc = start + i + 1;
                groups_[index][loc].cond_update(groups_[index + 1][i], cur_cond, circ_);
            }
		}
		for (int i = 0; i < 4 * total; i++) {
            groups_[index + 1][i].cond_update(groups_[index + 1][i + total], cond, circ_);
		}
		auto pos0_add_total = circ_->PutADDGate(pos0, s_total);
		auto pos1_sub_total = circ_->PutSUBGate(pos1, s_total);
		group_pos_[index] = circ_->PutMUXGate(pos0_add_total, group_pos_[index], cond);
		group_pos_[index + 1] = circ_->PutMUXGate(pos1_sub_total, group_pos_[index + 1], cond);
	}
};
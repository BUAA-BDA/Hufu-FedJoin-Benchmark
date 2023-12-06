#include "sort_merge_join.h"
#include "obliv_stack.h"

void SortMergeJoin::join() {
	cout << "===== Sort Merge Join =====" << endl;
    party_->reset();
	circ_ = party_->get_circuit(S_BOOL);
    auto s_true = circ_->PutINGate(ONE, 32, SERVER);
	auto s_false = circ_->PutINGate(ZERO, 32, SERVER);
	auto s_zero = s_false, s_one = s_true;
	auto s_nil = circ_->PutINGate(NIL, 32, SERVER);
    OblivStack<SharedTuple> s1(party_->get_role(), party_->get_abyparty(), circ_, op_cnt_);
    OblivStack<SharedTuple> s2(party_->get_role(), party_->get_abyparty(), circ_, op_cnt_);
    OblivStack<SharedTuple> s2_bk(party_->get_role(), party_->get_abyparty(), circ_, op_cnt_);
    OblivStack<SharedTuple> res(party_->get_role(), party_->get_abyparty(), circ_, op_cnt_);
    for (int i = size_s_ - 1; i >= 0; i--) {
        auto k = circ_->PutINGate(relation_[i].k, 32, SERVER);
		auto v = circ_->PutINGate(relation_[i].d, 32, SERVER);
		auto ind = circ_->PutINGate(ind_vec_[i], 32, SERVER);
        vector<share*> inds = {ind};
        s1.push(s_true, SharedTuple{k, v, inds});
    }
    for (int i = size_c_ - 1; i >= 0; i--) {
        auto k = circ_->PutINGate(relation_[i].k, 32, CLIENT);
		auto v = circ_->PutINGate(relation_[i].d, 32, CLIENT);
		auto ind = circ_->PutINGate(ind_vec_[i], 32, CLIENT);
		vector<share*> inds = {ind};
        s2.push(s_true, SharedTuple{k, v, inds});
    }

    auto back = s_false;
	auto r1_cur = s1.top(s_true);
	auto r2_cur = s2.top(s_true);

	for (int i = 0; i < size_res_ * 2; i++) {
		// logic of global
		auto eof1 = circ_->PutEQGate(r1_cur.k, s_nil);
		auto eof2 = circ_->PutEQGate(r2_cur.k, s_nil);
		auto global = circ_->PutANDGate(eof1, eof2);
		global = circ_->PutXORGate(s_true, global);

		// prepare all the condition
		auto forward = circ_->PutXORGate(s_true, back);
		back = circ_->PutANDGate(back, global);

		forward = circ_->PutANDGate(forward, global);
		auto r1_cur_lt_r2_cur = circ_->PutGTGate(r2_cur.k, r1_cur.k);
		r1_cur_lt_r2_cur = circ_->PutANDGate(r1_cur_lt_r2_cur, forward);
		auto r1_cur_ge_r2_cur = circ_->PutXORGate(s_true, r1_cur_lt_r2_cur);
		r1_cur_ge_r2_cur = circ_->PutANDGate(r1_cur_ge_r2_cur, forward);
		auto r1_cur_eq_r2_cur = circ_->PutEQGate(r1_cur.k, r2_cur.k);
		r1_cur_eq_r2_cur = circ_->PutANDGate(r1_cur_eq_r2_cur, forward);

		// logic of backward
		auto s2_bk_top = s2_bk.pop(back, "back round" + to_string(i));
		r2_cur.cond_update(s2_bk_top, back, circ_);
		s2.push(back, s2_bk_top, "back round" + to_string(i));
		auto r2_cur_ind_eq1 = circ_->PutEQGate(r2_cur.inds[0], s_one);
		r2_cur_ind_eq1 = circ_->PutANDGate(r2_cur_ind_eq1, back);
		back = circ_->PutMUXGate(s_false, back, r2_cur_ind_eq1);

		// logic of r1_cur < r2_cur
		s1.pop(r1_cur_lt_r2_cur, "forward round" + to_string(i));
		auto s1_top = s1.top(r1_cur_lt_r2_cur, "forward round" + to_string(i));
		r1_cur.cond_update(s1_top, r1_cur_lt_r2_cur, circ_);
		auto r1_cur_ind_eq1 = circ_->PutEQGate(r1_cur.inds[0], s_one);
		r1_cur_ind_eq1 = circ_->PutANDGate(r1_cur_ind_eq1, r1_cur_lt_r2_cur);
		back = circ_->PutMUXGate(s_true, back, r1_cur_ind_eq1);

		// logic of r1_cur >= r2_cur
		vector<share*> inds = {r2_cur.d};
		res.push(r1_cur_eq_r2_cur, SharedTuple{r1_cur.k, r1_cur.d, inds});
		auto s2_top = s2.pop(r1_cur_ge_r2_cur, "forward round" + to_string(i));
		s2_bk.push(r1_cur_ge_r2_cur, s2_top, "forward round" + to_string(i));
		auto s2_top_new = s2.top(r1_cur_ge_r2_cur, "forward round" + to_string(i));
		r2_cur.cond_update(s2_top_new, r1_cur_ge_r2_cur, circ_);
	}

#ifdef DEBUG
	s1.reveal_log();
	s2.reveal_log();
	s2_bk.reveal_log();
#endif

	vector<SharedTuple> result;
	for (int i = 0; i < size_res_; i++) {
		auto r = res.pop(s_true);
		if (SHARED) {
			r.k = circ_->PutSharedOUTGate(r.k);
			r.d = circ_->PutSharedOUTGate(r.d);
			r.inds[0] = circ_->PutSharedOUTGate(r.inds[0]);
		}
		else {
			r.k = circ_->PutOUTGate(r.k, ALL);
			r.d = circ_->PutOUTGate(r.d, ALL);
			r.inds[0] = circ_->PutOUTGate(r.inds[0], ALL);
		}
		res_.push_back(r);
	}
	cout << "begin Exec" << endl;
	party_->exec();
	int cnt = 1;
	for (int i = size_res_ - 1; i >= 0; i--) {
		if (res_[i].k->get_clear_value<uint32_t>() != NIL) {
			cout << "(" << cnt << ") " << res_[i].k->get_clear_value<uint32_t>() << " " 
				<< res_[i].d->get_clear_value<uint32_t>() << " " 
				<< res_[i].inds[0]->get_clear_value<uint32_t>() << endl;
			cnt++;
		}
	}
#ifdef DEBUG
	s1.print_log();
	s2.print_log();
	s2_bk.print_log();
#endif
	cout << "sortmerge comm: " << party_->get_comm() << endl;
    party_->clear_comm();
}

void SortMergeJoin::join_PKFK() {
	cout << "===== Sort Merge Join (PK-FK) =====" << endl;
    party_->reset();
	circ_ = party_->get_circuit(S_BOOL);
    auto s_true = circ_->PutINGate(ONE, 32, SERVER);
	auto s_false = circ_->PutINGate(ZERO, 32, SERVER);
	auto s_zero = s_false, s_one = s_true;
	auto s_nil = circ_->PutINGate(NIL, 32, SERVER);
    OblivStack<SharedTuple> s1(party_->get_role(), party_->get_abyparty(), circ_, op_cnt_);
    OblivStack<SharedTuple> s2(party_->get_role(), party_->get_abyparty(), circ_, op_cnt_);
    OblivStack<SharedTuple> res(party_->get_role(), party_->get_abyparty(), circ_, op_cnt_);
    for (int i = size_s_ - 1; i >= 0; i--) {
        auto k = circ_->PutINGate(relation_[i].k, 32, SERVER);
		auto v = circ_->PutINGate(relation_[i].d, 32, SERVER);
		auto ind = circ_->PutINGate(ind_vec_[i], 32, SERVER);
        vector<share*> inds = {ind};
        s1.push(s_true, SharedTuple{k, v, inds});
    }
    for (int i = size_c_ - 1; i >= 0; i--) {
        auto k = circ_->PutINGate(relation_[i].k, 32, CLIENT);
		auto v = circ_->PutINGate(relation_[i].d, 32, CLIENT);
		auto ind = circ_->PutINGate(ind_vec_[i], 32, CLIENT);
		vector<share*> inds = {ind};
        s2.push(s_true, SharedTuple{k, v, inds});
    }

	auto r1_cur = s1.top(s_true);
	auto r2_cur = s2.top(s_true);

	for (int i = 0; i < size_res_ * 2; i++) {
		// logic of global
		auto eof1 = circ_->PutEQGate(r1_cur.k, s_nil);
		auto eof2 = circ_->PutEQGate(r2_cur.k, s_nil);
		auto global = circ_->PutANDGate(eof1, eof2);
		global = circ_->PutXORGate(s_true, global);

		// prepare all the condition
		auto r1_cur_lt_r2_cur = circ_->PutGTGate(r2_cur.k, r1_cur.k);
		r1_cur_lt_r2_cur = circ_->PutANDGate(r1_cur_lt_r2_cur, global);
		auto r1_cur_ge_r2_cur = circ_->PutXORGate(s_true, r1_cur_lt_r2_cur);
		r1_cur_ge_r2_cur = circ_->PutANDGate(r1_cur_ge_r2_cur, global);
		auto r1_cur_eq_r2_cur = circ_->PutEQGate(r1_cur.k, r2_cur.k);
		r1_cur_eq_r2_cur = circ_->PutANDGate(r1_cur_eq_r2_cur, global);

		// logic of r1_cur < r2_cur
		s1.pop(r1_cur_lt_r2_cur, "round" + to_string(i));
		auto s1_top = s1.top(r1_cur_lt_r2_cur, "round" + to_string(i));
		r1_cur.cond_update(s1_top, r1_cur_lt_r2_cur, circ_);

		// logic of r1_cur >= r2_cur
		vector<share*> inds = {r2_cur.d};
		res.push(r1_cur_eq_r2_cur, SharedTuple{r1_cur.k, r1_cur.d, inds});
		auto s2_top = s2.pop(r1_cur_ge_r2_cur, "round" + to_string(i));
		auto s2_top_new = s2.top(r1_cur_ge_r2_cur, "round" + to_string(i));
		r2_cur.cond_update(s2_top_new, r1_cur_ge_r2_cur, circ_);
	}

#ifdef DEBUG
	s1.reveal_log();
	s2.reveal_log();
#endif

	vector<SharedTuple> result;
	for (int i = 0; i < size_res_; i++) {
		auto r = res.pop(s_true);
		if (SHARED) {
			r.k = circ_->PutSharedOUTGate(r.k);
			r.d = circ_->PutSharedOUTGate(r.d);
			r.inds[0] = circ_->PutSharedOUTGate(r.inds[0]);
		}
		else {
			r.k = circ_->PutOUTGate(r.k, ALL);
			r.d = circ_->PutOUTGate(r.d, ALL);
			r.inds[0] = circ_->PutOUTGate(r.inds[0], ALL);
		}
		res_.push_back(r);
	}
	cout << "begin Exec" << endl;
	party_->exec();
	int cnt = 1;
	for (int i = size_res_ - 1; i >= 0; i--) {
		if (res_[i].k->get_clear_value<uint32_t>() != NIL) {
			cout << "(" << cnt << ") " << res_[i].k->get_clear_value<uint32_t>() << " " 
				<< res_[i].d->get_clear_value<uint32_t>() << " " 
				<< res_[i].inds[0]->get_clear_value<uint32_t>() << endl;
			cnt++;
		}
	}
#ifdef DEBUG
	s1.print_log();
	s2.print_log();
#endif
	cout << "sortmerge comm: " << party_->get_comm() << endl;
    party_->clear_comm();
}

void SortMergeJoin::theta_join() {
	cout << "===== Sort Merge Join =====" << endl;
    party_->reset();
	circ_ = party_->get_circuit(S_BOOL);
    auto s_true = circ_->PutINGate(ONE, 32, SERVER);
	auto s_false = circ_->PutINGate(ZERO, 32, SERVER);
	auto s_zero = s_false, s_one = s_true;
	auto s_nil = circ_->PutINGate(NIL, 32, SERVER);
    OblivStack<SharedTuple> s1(party_->get_role(), party_->get_abyparty(), circ_, op_cnt_);
    OblivStack<SharedTuple> s2(party_->get_role(), party_->get_abyparty(), circ_, op_cnt_);
    OblivStack<SharedTuple> s2_bk(party_->get_role(), party_->get_abyparty(), circ_, op_cnt_);
    OblivStack<SharedTuple> res(party_->get_role(), party_->get_abyparty(), circ_, op_cnt_);
    for (int i = size_s_ - 1; i >= 0; i--) {
		cout << i << endl;
        auto k = circ_->PutINGate(relation_[i].k, 32, SERVER);
		auto v = circ_->PutINGate(relation_[i].d, 32, SERVER);
		vector<share*> inds = {s_zero};
        s1.push(s_true, SharedTuple{k, v, inds});
    }
    for (int i = size_c_ - 1; i >= 0; i--) {
        auto k = circ_->PutINGate(relation_[i].k, 32, CLIENT);
		auto v = circ_->PutINGate(relation_[i].d, 32, CLIENT);
		vector<share*> inds = {s_zero};
        s2.push(s_true, SharedTuple{k, v, inds});
    }

	cout << "here" << endl;

	vector<share*> inds = {s_one};
    s2_bk.push(s_true, SharedTuple{s_zero, s_zero, inds});

    auto back = s_false;
	auto r1_cur = s1.top(s_true);
	auto r2_cur = s2.top(s_true);

#ifdef DEBUG
	vector<share*> global_list;
	vector<share*> back_list;
	vector<share*> forward_list;
#endif

	for (int i = 0; i < size_res_ * 2; i++) {
		cout << i << endl;
		// prepare all the condition
		auto eof1 = circ_->PutEQGate(r1_cur.k, s_nil);
		auto eof2 = circ_->PutEQGate(r2_cur.k, s_nil);
		auto global = circ_->PutXORGate(eof1, s_true);
		auto forward = circ_->PutXORGate(s_true, back);
		
		back = circ_->PutANDGate(back, global);
		forward = circ_->PutANDGate(forward, global);
		auto not_eof2 = circ_->PutXORGate(eof2, s_true);
		auto turn_back = circ_->PutANDGate(eof2, forward);
		auto turn_forward = circ_->PutEQGate(r2_cur.inds[0], s_one);
		turn_forward = circ_->PutANDGate(turn_forward, back);
		forward = circ_->PutANDGate(forward, not_eof2);

#ifdef DEBUG
		global_list.push_back(global);
		back_list.push_back(back);
		forward_list.push_back(forward);
#endif
		auto r1_cur_lt_r2_cur = circ_->PutGTGate(r2_cur.k, r1_cur.k);
		auto r1_cur_ge_r2_cur = circ_->PutXORGate(s_true, r1_cur_lt_r2_cur);
		r1_cur_lt_r2_cur = circ_->PutANDGate(r1_cur_lt_r2_cur, forward);
		r1_cur_ge_r2_cur = circ_->PutANDGate(r1_cur_ge_r2_cur, forward);

	// logic of backward
		// general
		auto s2_bk_top = s2_bk.pop(back, "back round" + to_string(i));
		r2_cur.cond_update(s2_bk_top, back, circ_);
		s2.push(back, s2_bk_top, "back round" + to_string(i));

		// turn forward
		back = circ_->PutMUXGate(s_false, back, turn_forward);
		s1.pop(turn_forward, "back round" + to_string(i));
		auto s1_top = s1.top(turn_forward, "back round" + to_string(i));
		r1_cur.cond_update(s1_top, turn_forward, circ_);

	// logic of forward
		// turn back
		back = circ_->PutMUXGate(s_true, back, turn_back);

		// r1_cur < r2_cur
		vector<share*> inds = {r2_cur.k};
		res.push(r1_cur_lt_r2_cur, SharedTuple{r1_cur.k, r1_cur.d, inds});

		// r1_cur >= r2_cur
		r2_cur.inds[0] = circ_->PutMUXGate(s_one, r2_cur.inds[0], r1_cur_ge_r2_cur);
		
		// general
		s2_bk.push(forward, r2_cur, "forward round" + to_string(i));
		s2.pop(forward, "forward round" + to_string(i));
		auto s2_top = s2.top(forward, "forward round" + to_string(i));
		r2_cur.cond_update(s2_top, forward, circ_);
	}

#ifdef DEBUG
	s1.reveal_log();
	s2.reveal_log();
	s2_bk.reveal_log();
	for (int i = 0; i < global_list.size(); i++) {
		global_list[i] = circ_->PutOUTGate(global_list[i], ALL);
		back_list[i] = circ_->PutOUTGate(back_list[i], ALL);
		forward_list[i] = circ_->PutOUTGate(forward_list[i], ALL);
	}
#endif

	vector<SharedTuple> result;
	for (int i = 0; i < size_res_; i++) {
		auto r = res.pop(s_true);
		r.k = circ_->PutOUTGate(r.k, ALL);
		r.d = circ_->PutOUTGate(r.d, ALL);
		r.inds[0] = circ_->PutOUTGate(r.inds[0], ALL);
		res_.push_back(r);
	}
	cout << "begin Exec" << endl;
	party_->exec();
	int cnt = 1;
	for (int i = size_res_ - 1; i >= 0; i--) {
		if (res_[i].k->get_clear_value<uint32_t>() != NIL) {
			cout << "(" << cnt << ") " << res_[i].k->get_clear_value<uint32_t>() << " " 
				<< res_[i].d->get_clear_value<uint32_t>() << " " 
				<< res_[i].inds[0]->get_clear_value<uint32_t>() << endl;
			cnt++;
		}
	}
#ifdef DEBUG
	s1.print_log();
	s2.print_log();
	s2_bk.print_log();
	for (int i = 0; i < global_list.size(); i++) {
		cout << i << " " << global_list[i]->get_clear_value<uint32_t>() << " "
			<< back_list[i]->get_clear_value<uint32_t>() << " " 
			<< forward_list[i]->get_clear_value<uint32_t>() << endl;
	}
#endif
	cout << "sortmerge comm: " << party_->get_comm() << endl;
    party_->clear_comm();
}
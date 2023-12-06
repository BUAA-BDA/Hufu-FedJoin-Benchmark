#include "nest_loop_join.h"
#include "obliv_join.h"
#include "fast_join.h"
#include "hash_join.h"
#include "sort_merge_join.h"
#include "utils.h"
#include <iostream>
#include <netinet/in.h>
#include <netdb.h>
using namespace std;

int main(int argc, char* argv[]) {
	if (argc < 5) {
		cout << "parameters wrong!" << endl;
		exit(0);
	}
	e_role role = SERVER;

	string file_name = argv[2];
	int size_s = atoi(argv[3]);
	int size_c = atoi(argv[4]);

	int size = size_s;
	if (argv[1][0] == 'C') {
		role = CLIENT;
		size = size_c;
	}

	string address = "127.0.0.1";
	uint16_t port = 7766;
	string mode = "ALL";
	uint32_t other_para = 0;
	
	if (argc > 5) {
		mode = argv[5];
	}

	if (argc > 6) {
		other_para = atoi(argv[6]);
	}

	if (argc > 7) {
		address = argv[7];
		port = atoi(argv[8]);
	}

	cout << role << " " << file_name << " " << size_s << "*" << size_c << 
		" " << address << ":" << port << endl;

	vector<Tuple> relation;
	readRelation(file_name, relation, size);
	Party* party = new Party(role, address, port);

	double start = wallClock();

	if (mode == "ALL") {
		auto j1 = new NestLoopJoin(party, relation, size_s, size_c, other_para);
		j1->join();
		auto j2 = new OblivJoin(party, relation, size_s, size_c, other_para);
		j2->join();
		auto j3 = new FastJoin(party, relation, size_s, size_c);
		j3->join();
		auto j4 = new HashJoin(party, relation, size_s, size_c, other_para);
		j4->join();
		auto j5 = new SortMergeJoin(party, relation, size_s, size_c, other_para);
		j5->join();
	}
	else if (mode == "NESTLOOP") {
		auto j1 = new NestLoopJoin(party, relation, size_s, size_c, other_para);
		j1->join_simd();
	}
	else if (mode == "NESTLOOP_THETA") {
		auto j1 = new NestLoopJoin(party, relation, size_s, size_c, other_para);
		j1->theta_join_simd();
	}
	else if (mode == "NESTLOOP_ORAM") {
		auto j1 = new NestLoopJoin(party, relation, size_s, size_c, other_para);
		j1->join_oram();
	}
	else if (mode == "OBLIV") {
		auto j2 = new OblivJoin(party, relation, size_s, size_c, other_para);
		j2->join();
	}
	else if (mode == "OBLIV_PK") {
		auto j2 = new OblivJoin(party, relation, size_s, size_c, other_para);
		j2->join_PKFK();
	}
	else if (mode == "FAST") {
		auto j3 = new FastJoin(party, relation, size_s, size_c);
		j3->join();
	}
	else if (mode == "HASH") {
		auto j4 = new HashJoin(party, relation, size_s, size_c, other_para);
		j4->join();
	}
	else if (mode == "SORTMERGE") {
		auto j5 = new SortMergeJoin(party, relation, size_s, size_c, other_para);
		j5->join();
	}
	else if (mode == "SORTMERGE_PK") {
		auto j5 = new SortMergeJoin(party, relation, size_s, size_c, other_para);
		j5->join_PKFK();
	}
	else if (mode == "SORTMERGE_THETA") {
		auto j5 = new SortMergeJoin(party, relation, size_s, size_c, other_para);
		j5->theta_join();
	}
	else {
		cout << "mode not support" << endl;
	}

	double end = wallClock();
	cout << "time: " << end - start << endl;
}

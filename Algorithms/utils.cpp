#include "utils.h"

using namespace std;

void readRelation(string file_name, vector<Tuple>& relation, int size) {
    fstream fin(file_name);
    for (int i = 0; i < size; i++) {
        uint32_t k, d;
        fin >> k >> d;
        relation.push_back(*(new Tuple(k, d)));
    }
    fin.close();
}

double wallClock() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec + 1e-9 * t.tv_nsec;
}
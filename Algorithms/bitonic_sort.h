#include "utils.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
using namespace std;
 
class BitonicSort {
public:
    BitonicSort(Party* party, vector<Tuple>& input, Tuple null_value):
        party_(party), input_(input) {
        size_ = prev_pow_two(input.size()) * 2;
		middle_ = vector<Tuple>(input_);
		while (middle_.size() < size_) {
			middle_.push_back(null_value);
		}
    }

    void sort(void (*cmp)(Party*, vector<Tuple>&, vector<Tuple>&, vector<Tuple>&,vector<Tuple>&)) {        
		for (int region = size_ / 2; region >= 2; region /= 2) {
			int gap = size_ / region / 2;
			for (int l = gap; l >= 1; l /= 2) {
				vector<Tuple> in1, in2;
				for (int re = 0; re < region * (gap / l); re++) {
					for (int i = 0; i < l; i++) {
						int loc = re * l * 2 + i;
						in1.push_back(middle_[loc]);
						in2.push_back(middle_[loc + l]);
					}
				}

				vector<Tuple> small, big;
				cmp(party_, in1, in2, small, big);

				int pos = 0;
				for (int re = 0; re < region * (gap / l); re++) {
					for (int i = 0; i < l; i++) {
						int loc = re * l * 2 + i;
						if ((re / (gap / l)) % 2 == 0) {
							middle_[loc] = small[pos];
							middle_[loc + l] = big[pos];
						}
						else {
							middle_[loc + l] = small[pos];
							middle_[loc] = big[pos];
						}
						pos++;
					}
				}
			}
		}
		
	    int region = 1;
	    for (int l = size_ / 2; l >= 1; l /= 2) {
		    vector<Tuple> in1, in2;
		    for (int re = 0; re < region; re++) {
			    for (int i = 0; i < l; i++) {
				    int loc = re * l * 2 + i;
				    in1.push_back(middle_[loc]);
				    in2.push_back(middle_[loc + l]);
			    }
		    }

		    vector<Tuple> small, big;
			cmp(party_, in1, in2, small, big);

		    int pos = 0;
		    for (int re = 0; re < region; re++) {
			    for (int i = 0; i < l; i++) {
				    int loc = re * l * 2 + i;
				    middle_[loc] = small[pos];
				    middle_[loc + l] = big[pos];
				    pos++;
			    }
		    }
		    region *= 2;
	    }
		party_->reset();
    }

    vector<Tuple> get_output() {
		auto origin_size = input_.size();
		while (middle_.size() > input_.size()) {
			middle_.pop_back();
		}
        return middle_;
    }

private:
	Party* party_;
    uint32_t size_;
    vector<Tuple> input_;
	vector<Tuple> middle_;
};
#ifndef PROJECT_BIN_PACKER_H
#define PROJECT_BIN_PACKER_H

#include <vector>
#include <algorithm>
#include "pattern_database_interface.h"
#include "../abstract_task.h"

using namespace std;

namespace pdbs {
     class BinPacker {
        public:
            //this method produce a distribution of variables using a bin packing algorithm (Virtual)
            Pattern transform_to_pattern_normal_form(
		vector<bool> &bitvector) const {
            	Pattern pattern;
            	for (size_t i = 0; i < bitvector.size(); ++i) {
                	if (bitvector[i]) pattern.push_back(i);
            	}
            	return pattern;
        	}
            virtual vector<vector<vector<bool>>> bin_packing(shared_ptr<AbstractTask> task) = 0;
            virtual ~BinPacker() {}
     };
}

#endif //PROJECT_BIN_PACKER_H

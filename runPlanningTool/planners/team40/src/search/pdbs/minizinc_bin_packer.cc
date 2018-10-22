#include "first_fit_decreasing_bin_packer.h"

#include <iostream>

#include "bin_packer.h"
#include "../task_proxy.h"
#include "../utils/timer.h"
#include "../utils/math.h"

using namespace pdbs;
using namespace std;

MinizincBinPacker::MinizincBinPacker(double pdb_size, int num_collections) :
        pdb_max_size (pdb_size),
        num_pbd_collections (num_collections) {
}

vector<vector<vector<bool>>> MinizincBinPacker::bin_packing(shared_ptr<AbstractTask> task) {

    vector<vector<vector<bool>>> pattern_collections;
    vector<vector<bool>> pattern_collection;
    double current_size;

    //int temp = rand()%(max_target_size-min_target_size+1);
    //temp += min_target_size;

    //pdb_max_size=9*pow(10,temp);
    //pdb_max_size=min(pdb_max_size,pow(10,initial_max_target_size));
    //pdb_max_size=max(pdb_max_size,pow(10,min_target_size));

    cout << "Starting bin packing Minizinc, pdb_max_size:" << pdb_max_size << endl;

    TaskProxy task_proxy(*task);
    VariablesProxy variables = task_proxy.get_variables();

    for (int i = 0; i < num_pbd_collections; ++i) {

        pattern_collections.clear();

        vector<pair<int,double>> remaining_vars;

        for (size_t i = 0; i < variables.size(); ++i) {
            if (variables[i].get_domain_size() <= pdb_max_size) {
                remaining_vars.push_back(make_pair(i, variables[i].get_domain_size()));
            }
        }

        //Init pattern
        vector<bool> pattern(variables.size(), false);
        current_size = 1;

        pattern_collections.push_back(pattern_collection);
    }

    cout << " binpacking time: " << utils::g_timer << " with " << pattern_collections.back().size() << endl;
    return pattern_collections;
}
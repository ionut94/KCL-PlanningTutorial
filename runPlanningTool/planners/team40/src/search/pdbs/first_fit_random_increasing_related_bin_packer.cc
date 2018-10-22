#include "first_fit_random_increasing_related_bin_packer.h"

#include <iostream>
#include <stdlib.h>

#include "bin_packer.h"
#include "../task_proxy.h"
#include "../causal_graph.h"
#include "../utils/timer.h"
#include "../utils/math.h"

using namespace pdbs;
using namespace std;

FirstFitRandomIncreasingRelatedBinPacker::FirstFitRandomIncreasingRelatedBinPacker(double pdb_size, int num_collections) :
        pdb_max_size (pdb_size),
        num_pbd_collections (num_collections) {
}

vector<vector<vector<bool>>> FirstFitRandomIncreasingRelatedBinPacker::bin_packing(shared_ptr<AbstractTask> task) {

    vector<vector<vector<bool>>> pattern_collections;
    vector<vector<bool>> pattern_collection;
    double current_size;

    //int temp = rand()%(max_target_size-min_target_size+1);
    //temp += min_target_size;

    //pdb_max_size=9*pow(10,temp);
    //pdb_max_size=min(pdb_max_size,pow(10,initial_max_target_size));
    //pdb_max_size=max(pdb_max_size,pow(10,min_target_size));

    //cout << "Starting bin packing First Fit Decresing, pdb_max_size:" << pdb_max_size << endl;

    TaskProxy task_proxy(*task);
    VariablesProxy variables = task_proxy.get_variables();

    const CausalGraph &cg = task_proxy.get_causal_graph();

    for (int i = 0; i < num_pbd_collections; ++i) {

        pattern_collections.clear();

        vector<pair<int,double>> remaining_vars;

        for (size_t i = 0; i < variables.size(); ++i) {
            if (variables[i].get_domain_size() <= pdb_max_size) {
                remaining_vars.push_back(make_pair(i, variables[i].get_domain_size()));
                cout << "var " << i << ": " << variables[i].get_domain_size() << endl;
            }
        }

        sort(remaining_vars.begin(), remaining_vars.end(), compare_domain_size_variable);

        //Init pattern
        vector<bool> pattern(variables.size(), false);
        current_size = 1;

        size_t pos = 0;
        size_t current_var = rand() % remaining_vars.size();

        while (remaining_vars.size() > 0) {

            if(utils::is_product_within_limit(current_size, remaining_vars[pos].second, pdb_max_size)) {
                current_var = remaining_vars[pos].first;
                current_size *= remaining_vars[pos].second;
                pattern[remaining_vars[pos].first] = true;

                remaining_vars.erase(remaining_vars.begin() + pos);

                const vector<int> &rel_vars = cg.get_eff_to_pre(current_var);
                for (size_t j = 0; j < rel_vars.size(); ++j) {
                    size_t k = 0;
                    while (k < remaining_vars.size()) {
                        if ((remaining_vars[k].first == rel_vars[j]) && (utils::is_product_within_limit(current_size, remaining_vars[k].second, pdb_max_size))) {
                            current_size *= remaining_vars[k].second;
                            pattern[remaining_vars[k].first] = true;
                            remaining_vars.erase(remaining_vars.begin() + k);
                            break;
                        }
                        k++;
                    }
                }
            }
            else {
                //Add pattern (bin)
                pattern_collection.push_back(pattern);

                vector<int> trans_pattern=transform_to_pattern_normal_form(pattern);

                //Init pattern
                pattern.clear();
                pattern.resize(variables.size(), false);
                current_size = 1;
                current_var = rand() % remaining_vars.size();
            }
        }

        if (current_size > 1) {
            //Add pattern (bin)
            pattern_collection.push_back(pattern);
            vector<int> trans_pattern=transform_to_pattern_normal_form(pattern);
        }

        sort(pattern_collection.begin(), pattern_collection.end(), compare_pattern_length);
        pattern_collections.push_back(pattern_collection);
    }

    //cout << " binpacking time: " << utils::g_timer << " with " << pattern_collections.back().size() << endl;
    return pattern_collections;
}

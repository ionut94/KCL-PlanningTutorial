#include "pattern_collection_generator_minizinc.h"
#include "pdb_factory.h"

using namespace std;
using namespace pdbs;

PatternCollectionGeneratorMinizinc::PatternCollectionGeneratorMinizinc(const options::Options &opts):
        pdb_factory (opts.get<shared_ptr<PDBFactory>>("pdb_type")),
        pdb_max_size(opts.get<int>("pdb_max_size")),
        num_collections(opts.get<int>("num_collections")),
        disjoint_patterns(opts.get<bool>("disjoint")),
        recompute_max_additive_subsets(opts.get<bool>("recompute_max_additive_subsets")),
        time_limit(opts.get<int>("time_limit")) {
}

PatternCollectionGeneratorMinizinc::~PatternCollectionGeneratorMinizinc() {

}

void PatternCollectionGeneratorMinizinc::bin_packing() {

    TaskProxy task_proxy(*task);
    vector<int> variable_ids;

    variable_ids.reserve(task_proxy.get_variables().size());

    for (size_t i = 0; i < task_proxy.get_variables().size(); ++i) {
        variable_ids.push_back(i);
    }
}

PatternCollectionInformation PatternCollectionGeneratorMinizinc::generate(shared_ptr<AbstractTask> task) {
    utils::Timer timer;
}
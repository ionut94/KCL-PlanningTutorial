#include "pattern_collection_generator_first_fit_decreasing.h"

#include "first_fit_decreasing_bin_packer.h"
#include "pattern_database_interface.h"
#include "zero_one_pdbs.h"

#include "../utils/timer.h"
#include "../utils/math.h"
#include "../task_proxy.h"
#include "../causal_graph.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../globals.h"
#include "first_fit_decreasing_bin_packer.h"
#include "first_fit_decreasing_related_bin_packer.h"

#include <unordered_set>

using namespace std;
using namespace pdbs;

PatternCollectionGeneratorFirstFitDecreasing::PatternCollectionGeneratorFirstFitDecreasing(const options::Options &opts):
        pdb_factory(opts.get<shared_ptr<PDBFactory>>("pdb_factory")),
        pdb_max_size(opts.get<double>("pdb_max_size")),
        num_collections(opts.get<int>("num_collections")),
        use_norm_dist(opts.get<bool>("use_norm_dist")),
        disjoint_patterns(opts.get<bool>("disjoint")),
        recompute_max_additive_subsets(opts.get<bool>("recompute_max_additive_subsets")) {

    num_collections=1;
    result = make_shared<PatternCollectionInformation>(task, make_shared<PatternCollection>());

    if(pdb_factory->name() == "symbolic") pdb_max_size = 2*pow(10,5);
    else pdb_max_size = 2*pow(10,4);

    bin_packer_algorithm = new FirstFitDecreasingRelatedBinPacker(pdb_max_size, num_collections);
}

Pattern PatternCollectionGeneratorFirstFitDecreasing::transform_to_pattern_normal_form(
        const vector<bool> &bitvector) const {
    Pattern pattern;
    for (size_t i = 0; i < bitvector.size(); ++i) {
        if (bitvector[i]) pattern.push_back(i);
    }
    return pattern;
}

double PatternCollectionGeneratorFirstFitDecreasing::get_pattern_size(Pattern pattern) {

    if(pattern.size()==0) return 0;

    // test if the pattern respects the memory limit
    double mem = 1;

    for (size_t j = 0; j < pattern.size(); ++j)
        mem *=  g_variable_domain[pattern[j]];

    return mem;
}

void PatternCollectionGeneratorFirstFitDecreasing::remove_irrelevant_variables(
        Pattern &pattern, shared_ptr<AbstractTask> task) const {

    TaskProxy task_proxy(*task);

    unordered_set<int> in_original_pattern(pattern.begin(), pattern.end());
    unordered_set<int> in_pruned_pattern;

    vector<int> vars_to_check;
    for (FactProxy goal : task_proxy.get_goals()) {
        int var_id = goal.get_variable().get_id();
        if (in_original_pattern.count(var_id)) {
            // Goals are causally relevant.
            vars_to_check.push_back(var_id);
            in_pruned_pattern.insert(var_id);
        }
    }

    while (!vars_to_check.empty()) {
        int var = vars_to_check.back();
        vars_to_check.pop_back();
        /*
          A variable is relevant to the pattern if it is a goal variable or if
          there is a pre->eff arc from the variable to a relevant variable.
          Note that there is no point in considering eff->eff arcs here.
        */
        const CausalGraph &cg = task_proxy.get_causal_graph();

        const vector<int> &rel = cg.get_eff_to_pre(var);
        for (size_t i = 0; i < rel.size(); ++i) {
            int var_no = rel[i];
            if (in_original_pattern.count(var_no) &&
                !in_pruned_pattern.count(var_no)) {
                // Parents of relevant variables are causally relevant.
                vars_to_check.push_back(var_no);
                in_pruned_pattern.insert(var_no);
            }
        }
    }

}

PatternCollectionInformation PatternCollectionGeneratorFirstFitDecreasing::generate(shared_ptr<AbstractTask> task) {

    utils::Timer timer;

    TaskProxy task_proxy(*task);

    pattern_collections = bin_packer_algorithm->bin_packing(task);

    for (const auto &collection : pattern_collections) {

        PatternCollection pattern_collection;

        for (const vector<bool> &bitvector : collection) {
            Pattern pattern = transform_to_pattern_normal_form(bitvector);
            remove_irrelevant_variables(pattern, task);
            pattern_collection.push_back(pattern);
        }

        ZeroOnePDBs candidate(task_proxy, pattern_collection, *pdb_factory);
        result->include_additive_pdbs(pdb_factory->terminate_creation(candidate.get_pattern_databases()));
    }

    result->set_dead_ends(pdb_factory->get_dead_ends());

    cout <<"Finished Pattern generation (Edelkamp (Martinez)) time: " << timer <<",Peak Memory:"<<utils::get_peak_memory_in_kb()<<",current_memory:"<<utils::get_current_memory_in_kb()<<flush<<endl;

    return *result;
}

static shared_ptr<PatternCollectionGenerator> _parse(OptionParser &parser) {
    parser.document_synopsis(
            "First Fit Decreasing Patterns",
            "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "not supported");
    parser.document_language_support("axioms", "not supported");
    parser.document_note(
            "Note",
            "This pattern generation method uses the zero/one pattern database heuristic.");
    parser.document_note(
            "Implementation Notes",
            "",
            true);
    parser.add_option<double>(
            "pdb_max_size",
            "maximal number of states per pattern database ",
            "50000",
            Bounds("1", "infinity"));
    parser.add_option<int>(
            "num_collections",
            "number of pattern collections to maintain in the genetic "
                    "algorithm (population size)",
            "5",
            Bounds("1", "infinity"));
    parser.add_option<bool>(
            "disjoint",
            "consider a pattern collection invalid (giving it very low "
                    "fitness) if its patterns are not disjoint",
            "false");
    parser.add_option<bool>(
            "hybrid_pdb_size",
            "mix pdb_sizes according to generation time",
            "true");
    parser.add_option<shared_ptr<PDBFactory>>(
            "pdb_factory",
            "See detailed documentation for pdb factories. ",
            "symbolic");
    parser.add_option<bool>(
            "recompute_max_additive_subsets",
            "attempts to recompute max additive subsets after generating all patterns",
            "false");
    parser.add_option<bool>(
            "create_perimeter",
            "whether to start with a perimeter",
            "false");
    parser.add_option<bool>(
            "use_norm_dist",
            "Whether to select PDB size log as a normal or uniform distribution",
            "true");

    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;

    return make_shared<PatternCollectionGeneratorFirstFitDecreasing>(opts);
}

ostream& operator<<(ostream& os, const vector<bool>& v){
    os << "[";
    for(size_t i=0; i<v.size(); ++i) {
        if(v.at(i)) os <<i<<",";
    }
    return os;
}


static PluginShared<PatternCollectionGenerator> _plugin("first_fit_decreasing", _parse);
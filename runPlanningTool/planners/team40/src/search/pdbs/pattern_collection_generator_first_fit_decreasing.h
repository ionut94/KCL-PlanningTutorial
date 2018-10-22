#ifndef PATTERN_COLLECTION_GENERATOR_FIRST_FIT_DECREASING_H
#define PATTERN_COLLECTION_GENERATOR_FIRST_FIT_DECREASING_H

#include <memory>
#include <vector>
#include <random>
#include "pdb_factory.h"
#include "pattern_generator.h"
#include "types.h"
#include "../options/option_parser.h"
#include "../options/options.h"
#include "bin_packer.h"

using namespace std;

namespace pdbs {

    class PatternCollectionGeneratorFirstFitDecreasing : public PatternCollectionGenerator {

    private:
        BinPacker *bin_packer_algorithm;

        std::shared_ptr<PDBFactory> pdb_factory;
        std::shared_ptr<AbstractTask> task;

        // Maximum number of states for each pdb
        double pdb_max_size;
        double min_size = 0;
        int num_collections;

        int max_target_size = 6;
        int initial_max_target_size = 6;
        int min_target_size = 4;

        bool use_norm_dist;

        /* Specifies whether patterns in each pattern collection need to be disjoint or not. */
        bool disjoint_patterns;
        //Whether to recompute the max_additive_subsets
        bool recompute_max_additive_subsets;

        // All current pattern collections.
        std::vector<std::vector<std::vector<bool>>> pattern_collections;
        // Final PDB
        std::shared_ptr<PatternCollectionInformation> result;

        void bin_packing(shared_ptr<AbstractTask> task);
        double get_pattern_size(Pattern pattern);

        Pattern transform_to_pattern_normal_form(
                const vector<bool> &bitvector) const;

        void remove_irrelevant_variables(
                Pattern &pattern,
                shared_ptr<AbstractTask> task) const;

        static bool compare_domain_size_variable(const pair<int,double> one, const pair<int,double> two) {
            return (one.second > two.second);
        }



    public:

        PatternCollectionGeneratorFirstFitDecreasing(const options::Options &opts);
        virtual ~PatternCollectionGeneratorFirstFitDecreasing() = default;
        virtual PatternCollectionInformation generate(std::shared_ptr<AbstractTask> task) override;
        virtual std::shared_ptr<PDBFactory> get_factory () override { return pdb_factory; }
    };
}

#endif //PATTERN_COLLECTION_GENERATOR_FIRST_FIT_DECREASING_H

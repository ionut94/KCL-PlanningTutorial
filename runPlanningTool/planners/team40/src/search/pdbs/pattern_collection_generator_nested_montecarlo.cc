

#include <memory>
#include <vector>
#include <random>
#include "../ss/ss_search.h"
#include "../global_state.h"
#include "group_zero_one_pdbs.h"

using namespace std;

namespace options {class Options;}

namespace pdbs {

    class PatternCollectionGeneratorFirstFitDecreasing : public PatternCollectionGenerator {

    private:

        std::shared_ptr<PDBFactory> pdb_factory;

        // Maximum number of states for each pdb
        const int pdb_max_size;
        const int num_collections;

        /* Specifies whether patterns in each pattern collection need to be disjoint or not. */
        const bool disjoint_patterns;

        const bool recompute_max_additive_subsets; //Whether to recompute the max_additive_subsets
        const double time_limit; //If greater than 1, runs the algorithm several times

        std::shared_ptr<AbstractTask> task;

        // All current pattern collections.
        std::vector<std::vector<std::vector<bool>>> pattern_collections;

        void bin_packing();

    public:

        PatternCollectionGeneratorFirstFitDecreasing(const options::Options &opts);
        virtual ~PatternCollectionGeneratorFirstFitDecreasing() = default;
        virtual PatternCollectionInformation generate(std::shared_ptr<AbstractTask> task) override;
        virtual std::shared_ptr<PDBFactory> get_factory () override { return pdb_factory; }
    };

}
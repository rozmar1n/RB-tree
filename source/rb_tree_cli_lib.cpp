#include "rb_tree_cli.hpp"

#include "rb_tree.hpp"

#include <cstddef>
#include <istream>
#include <ostream>

namespace {

void handle_query(rb::Tree<int>& tree,
                  int left,
                  int right,
                  bool& first_output,
                  std::ostream& output) {
    size_t result = 0;

    if (right > left) {
        const size_t right_rank = tree.rank_upper_bound(right);
        const size_t left_rank = tree.rank_upper_bound(left);
        if (right_rank > left_rank) {
            result = right_rank - left_rank;
        }
    }

    if (!first_output) {
        output << ' ';
    }
    output << result;
    first_output = false;
}

} // namespace
namespace rb {
int run_cli(std::istream& input,
            std::ostream& output,
            std::ostream& error) {
    rb::Tree<int> tree;
    bool first_output = true;

    char action = '\0';
    while (input >> action) {
        if (action == 'k') {
            int key = 0;
            if (!(input >> key)) {
                error << "Failed to read key value\n";
                return 1;
            }
            tree.insert(key);
        } else if (action == 'q') {
            int left = 0;
            int right = 0;
            if (!(input >> left >> right)) {
                error << "Failed to read query bounds\n";
                return 1;
            }
            handle_query(tree, left, right, first_output, output);
        } else {
            error << "Unknown command: " << action << '\n';
            return 1;
        }
    }

    if (!first_output) {
        output << '\n';
    }

    return 0;
} 
} //namespace rb

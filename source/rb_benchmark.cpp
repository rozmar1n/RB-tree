#include "rb_tree.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <iterator>

namespace {

struct Operation {
    char type; // 'k' or 'q'
    int a = 0;
    int b = 0;
};

struct Options {
    std::size_t operation_count = 100000;
    unsigned seed = 42;
    int max_value = 1000000;
    double insert_ratio = 0.5;
};

bool parse_argument(std::string_view arg,
                    std::string_view name,
                    std::string& value) {
    if (arg.size() <= name.size() || arg.compare(0, name.size(), name) != 0) {
        return false;
    }
    if (arg[name.size()] != '=') {
        return false;
    }
    value.assign(arg.begin() + static_cast<std::ptrdiff_t>(name.size()) + 1,
                 arg.end());
    return true;
}

Options parse_options(int argc, char* argv[]) {
    Options opts;
    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        std::string value;
        if (parse_argument(arg, "--ops", value)) {
            opts.operation_count = static_cast<std::size_t>(std::stoull(value));
        } else if (parse_argument(arg, "--seed", value)) {
            opts.seed = static_cast<unsigned>(std::stoul(value));
        } else if (parse_argument(arg, "--max", value)) {
            opts.max_value = std::stoi(value);
        } else if (parse_argument(arg, "--insert-ratio", value)) {
            opts.insert_ratio = std::clamp(std::stod(value), 0.0, 1.0);
        } else {
            std::cerr << "Unknown argument: " << arg << '\n';
            std::exit(1);
        }
    }
    return opts;
}

std::vector<Operation> build_workload(const Options& opts) {
    std::vector<Operation> operations;
    operations.reserve(opts.operation_count);

    std::mt19937 rng(opts.seed);
    std::uniform_real_distribution<double> ratio_dist(0.0, 1.0);
    std::uniform_int_distribution<int> value_dist(0, opts.max_value);

    for (std::size_t i = 0; i < opts.operation_count; ++i) {
        const double choice = ratio_dist(rng);
        if (choice < opts.insert_ratio) {
            operations.push_back(Operation{
                .type = 'k',
                .a = value_dist(rng),
            });
        } else {
            int left = value_dist(rng);
            int right = value_dist(rng);
            operations.push_back(Operation{
                .type = 'q',
                .a = left,
                .b = right,
            });
        }
    }

    return operations;
}

struct BenchmarkResult {
    std::chrono::duration<double> elapsed{};
    std::size_t checksum = 0;
};

BenchmarkResult run_rb_tree(const std::vector<Operation>& ops) {
    rb::Tree<int> tree;
    std::size_t checksum = 0;

    const auto start = std::chrono::steady_clock::now();
    for (const auto& op : ops) {
        if (op.type == 'k') {
            tree.insert(op.a);
        } else if (op.type == 'q') {
            size_t result = 0;
            if (op.b > op.a) {
                const size_t right_rank = tree.rank_upper_bound(op.b);
                const size_t left_rank = tree.rank_upper_bound(op.a);
                if (right_rank > left_rank) {
                    result = right_rank - left_rank;
                }
            }
            checksum += result;
        }
    }
    const auto end = std::chrono::steady_clock::now();

    return BenchmarkResult{
        .elapsed = end - start,
        .checksum = checksum,
    };
}

BenchmarkResult run_std_set(const std::vector<Operation>& ops) {
    std::set<int> tree;
    std::size_t checksum = 0;

    const auto start = std::chrono::steady_clock::now();
    for (const auto& op : ops) {
        if (op.type == 'k') {
            tree.insert(op.a);
        } else if (op.type == 'q') {
            size_t result = 0;
            if (op.b > op.a) {
                const auto left_it = tree.upper_bound(op.a);
                const auto right_it = tree.upper_bound(op.b);
                result = static_cast<std::size_t>(
                    std::distance(left_it, right_it));
            }
            checksum += result;
        }
    }
    const auto end = std::chrono::steady_clock::now();

    return BenchmarkResult{
        .elapsed = end - start,
        .checksum = checksum,
    };
}

void print_header(const Options& opts) {
    std::cout << "RB-tree vs std::set benchmark\n";
    std::cout << "Operations:    " << opts.operation_count << '\n';
    std::cout << "Insert ratio:  " << opts.insert_ratio << '\n';
    std::cout << "Value range:   [0, " << opts.max_value << "]\n";
    std::cout << "Seed:          " << opts.seed << "\n\n";
}

void print_result(std::string_view name, 
                  const BenchmarkResult& result) {
    std::cout << name << ": "
              << std::chrono::duration_cast<std::chrono::milliseconds>
        (        result.elapsed).count()
              << " ms, checksum " << result.checksum << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    const Options opts = parse_options(argc, argv);
    const auto workload = build_workload(opts);

    print_header(opts);

    const auto rb_result = run_rb_tree(workload);
    const auto std_result = run_std_set(workload);

    print_result("rb::Tree", rb_result);
    print_result("std::set", std_result);

    return 0;
}

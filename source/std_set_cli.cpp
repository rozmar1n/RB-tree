#include <iostream>
#include <set>

int main() {
    std::set<long long> tree;
    bool first_output = true;

    char command = '\0';
    while (std::cin >> command) {
        if (command == 'k') {
            long long key = 0;
            if (!(std::cin >> key)) {
                return 1;
            }
            tree.insert(key);
        } else if (command == 'q') {
            long long left = 0;
            long long right = 0;
            if (!(std::cin >> left >> right)) {
                return 1;
            }

            if (right < left) {
                if (!first_output) {
                    std::cout << ' ';
                }
                std::cout << 0;
                first_output = false;
                continue;
            }

            const auto begin = tree.lower_bound(left);
            const auto end = tree.upper_bound(right);
            const auto result = std::distance(begin, end);

            if (!first_output) {
                std::cout << ' ';
            }
            std::cout << result;
            first_output = false;
        } else {
            return 1;
        }
    }

    if (!first_output) {
        std::cout << '\n';
    }

    return 0;
}

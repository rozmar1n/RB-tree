#pragma once

#include <iosfwd>

namespace rb {
int run_cli_iter(std::istream& input,
                 std::ostream& output,
                 std::ostream& error);
} //namespace rb

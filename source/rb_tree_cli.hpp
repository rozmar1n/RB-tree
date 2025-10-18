#pragma once

#include <iosfwd>

namespace rb {
int run_cli(std::istream& input,
            std::ostream& output,
            std::ostream& error);
} //namespace rb

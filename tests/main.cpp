/**
 * @file main.cpp
 * @author Aakash Kaushik
 *
 * Main file/Entry point for Catch's main().
 *
 * mlpack is free software; you may redistribute it and/or modify it under the
 * terms of the 3-clause BSD license.  You should have received a copy of the
 * 3-clause BSD license along with mlpack.  If not, see
 * http://www.opensource.org/licenses/BSD-3-Clause for more information.
 */
#define CATCH_CONFIG_RUNNER // Define main() for Catch2 yourself.
#include "catch.hpp"
#include <armadillo>

int main(int argc, char* argv[])
{
  /**
   * Uncomment these three lines if you want to test with different random seeds
   * each run.  This is good for ensuring that a test's tolerance is sufficient
   * across many different runs.
   */
  // size_t seed = std::time(NULL);
  // srand((unsigned int) seed);
  // arma::arma_rng::set_seed(seed);

  return Catch::Session().run(argc, argv);
}

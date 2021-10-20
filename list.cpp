#include <iostream>
#include <string>
#include <vector>

#include "Blif.h"

std::vector<std::string> parseProgramArgument(int argc, char* argv[]);

int main(int argc, char* argv[]) {
  std::vector<std::string> args = parseProgramArgument(argc, argv);

  Blif* blif = nullptr;
  if (argc == 6 && args[1] == "-l") {
    // minimize latency - resource constrained scheduling
    blif = new Blif(args[2], std::stoi(args[3]), std::stoi(args[4]), std::stoi(args[5]));
    blif->ML_RCS();
  } else if (argc == 4 && args[1] == "-r") {
    // minimize resourece - latency constrained scheduling
    blif = new Blif(args[2], std::stoi(args[3]));
    blif->MR_LCS();
  } else {
    std::cerr << "Usage:\n"
      << "1. For ML-RCS: ./list -l <BLIF_FILE> <AND_CONSTRAINT> <OR_CONSTRAINT> <NOT_CONSTRAINT>\n"
      << "2. For MR-LCS: ./list -r <BLIF_FILE> <LATENCY_CONSTRAINT>\n";
    return 1;
  }

  std::cout << *blif;
  return 0;
}

std::vector<std::string> parseProgramArgument(int argc, char* argv[]) {
  std::vector<std::string> args;
  for (int i = 0; i < argc; ++i) {
    args.push_back(argv[i]);
  }

  return args;
}

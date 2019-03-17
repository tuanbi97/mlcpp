#include "datasetclasses.h"

namespace {
// std::vector<std::string> class_names = {"BG", "square", "triangle",
// "circle"};

std::vector<std::string> class_names = {"BG",
                                        "Sedan",
                                        "SUV",
                                        "Van",
                                        "Bus",
                                        "Pick-up Truck",
                                        "Semi-trailer Truck",
                                        "Motorcycle",
                                        "Bicycle",
                                        "Other"
                                      };
}  // namespace

const std::vector<std::string>& GetDatasetClasses() {
  return class_names;
}

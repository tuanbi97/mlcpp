#include "datasetclasses.h"

namespace {
// std::vector<std::string> class_names = {"BG", "square", "triangle",
// "circle"};

std::vector<std::string> class_names = {"bg",
                                        "sedan",
                                        "suv",
                                        "van",
                                        "bus",
                                        "pick-up truck",
                                        "semi-trailer truck",
                                        "motorcycle",
                                        "bicycle",
                                        "other"
                                      };
}  // namespace

const std::vector<std::string>& GetDatasetClasses() {
  return class_names;
}

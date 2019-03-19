#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include "loader.h"
#include <cstdint>
#include <experimental/filesystem>

const std::string VEHICLE_SOURCE = "vehicles";

namespace fs = std::experimental::filesystem;

struct BoundingBox {
  int32_t x{0};
  int32_t y{0};
  int32_t width{0};
  int32_t height{0};
};

class VehicleLoader : public Loader
{
  public:
    /// \brief Predefined field of csv files
    static const std::set<std::string> csv_fields;
    explicit VehicleLoader(const std::string &images_folder,
                           const std::string &annotations_file,
                           const std::vector<std::string> &classes);

    void LoadData() override;
    std::pair<std::vector<cv::Mat>, std::vector<std::int32_t>> LoadMask(const std::uint64_t &image_id) override;

    std::vector<BoundingBox> LoadBBoxes(const std::uint64_t &image_id);

    std::pair<std::uint32_t, std::vector<float>> LoadBBox(const std::uint64_t &image_id) override;
    std::pair<std::uint32_t, std::vector<float>> LoadRotatedBBox(const std::uint64_t &image_id) override;
    std::string ImageReference(const std::uint64_t &image_id) override;

  private:
    std::string images_folder_;
    std::string annotations_file_;
    void _AddClassesToBase(const std::vector<std::string> &classes);
    void _ParseContours(const std::string &string_points, ImageInfo &image_info);

}; //class VehicleDataLoader
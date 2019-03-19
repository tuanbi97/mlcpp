#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include <opencv2/opencv.hpp>
#include "util.h"

/// \brief Class information
struct ClassInfo
{
    std::string source;
    std::uint16_t id{0};
    std::string class_name;

    ClassInfo() = default;
    ClassInfo(std::string s, std::uint16_t i, std::string c) : source(s), id(i), class_name(c) {}
};

/// \brief Image information read from the
struct ImageInfo
{
    std::vector<std::vector<std::int32_t>> contours;
    std::string id{};
    std::string source{};
    std::string path{};
    std::string class_ids{};
    std::uint16_t width{0};
    std::uint16_t height{0};
    bool has_mask{false};

    void AddContourCoordinates(float v)
    {
        contours[contours.size() - 1].push_back(static_cast<std::int32_t>(v));
    }
};

class Loader
{
  public:
    Loader();
    void AddClass(const std::string &source, const std::uint16_t &class_id, const std::string &class_name);
    void AddImage(ImageInfo &image_info);

    void Prepare();
    std::size_t MapSourceClassId(const std::string &source_class_id);
    std::uint16_t SourceClassId(const std::uint16_t &class_id, const std::string &source);
    std::vector<std::uint64_t> GetImageIds() const;
    bool HasMask() const;
    void SetHasMask(bool &value);
    std::size_t GetImagesCount() const;
    std::string SourceImageLink(const std::uint64_t &image_id);
    cv::Mat LoadImage(const std::uint64_t &image_id);

    virtual std::string ImageReference(const std::uint64_t &image_id);
    virtual void LoadData() = 0;
    virtual std::pair<std::vector<cv::Mat>, std::vector<std::int32_t>> LoadMask(const std::uint64_t &image_id) = 0;
    virtual std::pair<std::uint32_t, std::vector<float>> LoadBBox(const std::uint64_t &image_id) = 0;
    virtual std::pair<std::uint32_t, std::vector<float>> LoadRotatedBBox(const std::uint64_t &image_id) = 0;

  protected:
    std::vector<std::uint64_t> image_ids_;
    std::vector<std::uint16_t> class_ids_;
    std::vector<ImageInfo> image_infos_;
    std::vector<ClassInfo> class_infos_;
    std::unordered_map<std::string, std::uint16_t> class_id_from_class_name_map_;
    std::unordered_map<std::string, std::vector<std::size_t>> source_class_ids_;
    std::unordered_map<std::string, std::uint16_t> class_from_source_map_;
    std::unordered_map<std::string, std::uint64_t> image_from_source_map_;
    std::vector<std::string> sources_;

    bool has_mask_;
    size_t num_classes_;
    size_t num_images_;
};
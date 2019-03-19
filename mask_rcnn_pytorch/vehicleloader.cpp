/*
 * VehicleLoader.h
 */

#include <iostream>
#include <algorithm>
#include <ctype.h>
#include "vehicleloader.h"
#include "imageutils.h"
#include "fileutil.h"
#include "util.h"
#include "csvparser.hpp"

const std::set<std::string> VehicleLoader::csv_fields{"file", "pts", "labels", "has_mask"};

void VehicleLoader::_ParseContours(const std::string &string_points, ImageInfo &image_info)
{
    std::cout<<"checkContours: " <<image_info.id << std::endl;
    int index = 0;
    int level = 0;
    int previous_pos = 0;
    for (int i = 0; i < string_points.length(); i++){
        if (string_points[i] == '['){
            level++;
            if (level == 2){
                previous_pos = i + 1;
            }
        }
        if (string_points[i] == ']'){
            level--;
            if (level == 1){
                std::string ss = string_points.substr(previous_pos, i - previous_pos);
                std::vector<std::int32_t> contour(0);
                image_info.contours.push_back(contour);
                if (image_info.id.compare("./data/training_img/300/DJI_0002_0025_1070.png") == 0){
                    std::cout << ss << std::endl;
                }
                std::vector<std::string> v_points_string = SplitString(ss, ',');
                for (auto &s : v_points_string)
                {
                    int num = std::strtof(s.c_str(), 0);
                    image_info.AddContourCoordinates(std::strtof(s.c_str(), 0));
                }
            }
        }
    }
    // std::cout << string_points << std::endl;
    // std::vector<std::string> v_string_points = SplitString(string_points, '|');
    // std::cout << v_string_points.size() <<  std::endl;
    // for (const auto &s : v_string_points)
    // {
    //     std::cout << s <<  std::endl;
    //     image_info.contours.emplace_back();
    //     // assumption is string_points are comma separated values of x,y like "0,0,1,1,2,3"
    //     std::vector<std::string> v_points_string = SplitString(s, ';');
    //     for (auto &s : v_points_string)
    //     {
    //         image_info.AddContourCoordinates(std::strtof(s.c_str(), 0));
    //     }
    // }
}

void VehicleLoader::_AddClassesToBase(const std::vector<std::string> &classes)
{
    // Add all the classes to the classes in the loader
    for (uint16_t i = 0; i < classes.size(); ++i)
    {
        std::string class_ = classes[i];
        std::transform(class_.begin(), class_.end(), class_.begin(), ::tolower);

        // background will be added by the base class
        if (class_ == "bg")
            continue;

        this->AddClass(VEHICLE_SOURCE, i, class_);
    }
}

VehicleLoader::VehicleLoader(const std::string &images_folder,
                             const std::string &annotations_file,
                             const std::vector<std::string> &classes)
{
    fs::path images_path(images_folder);
    if (!fs::exists(images_path))
        throw std::runtime_error(images_path.string() + " folder not found!!!");

    fs::path annotation_file_path(annotations_file);

    if (!fs::exists(annotation_file_path))
        throw std::runtime_error(annotation_file_path.string() + " file not found!!!");

    // check we have classes
    if (classes.empty())
    {
        throw std::runtime_error("No classes list found!!!");
    }

    images_folder_ = images_folder;
    annotations_file_ = annotations_file;

    // Add classes to the base class
    this->_AddClassesToBase(classes);
}

std::pair<std::vector<cv::Mat>, std::vector<std::int32_t>> VehicleLoader::LoadMask(const std::uint64_t &image_id)
{
    ImageInfo info = this->image_infos_[image_id];
    //std::cout << info.source << " " << info.path << " " << info.contours.size() << std::endl;

    assert(this->has_mask_);

    cv::Size size(info.width, info.height);

    std::vector<cv::Mat> masks(info.contours.size());

    std::size_t c_idx = 0;
    for (const auto &contour : info.contours)
    {
        //std::cout << contour.size() << std::endl;
        cv::Mat mask = ConvertPolygonToMask(contour, size);
        masks[c_idx] = mask;
        ++c_idx;
    }

    std::cout << info.class_ids << std::endl;

    std::vector<std::string> s_class_ids = SplitString(info.class_ids, ',');
    std::vector<std::int32_t> class_ids;
    for (auto &s : s_class_ids)
    {

        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        trim(s);
        s = s.substr(1, s.length() - 2);
        if (s.compare("pedestrian") == 0){
            s = "other";
        }

        if (this->class_id_from_class_name_map_.find(s) != this->class_id_from_class_name_map_.end())
            class_ids.push_back(this->class_id_from_class_name_map_[s]);
    }

    //        std::transform(s_class_ids.begin(), s_class_ids.end(), std::back_inserter(class_ids),
    //                        [](const std::string &str){ return std::stoi(str); });
    return std::make_pair(masks, class_ids);
}

std::vector<BoundingBox> VehicleLoader::LoadBBoxes(const std::uint64_t &image_id)
{
    std::cout<< image_id <<" " <<this->image_infos_.size() << std::endl;
    ImageInfo info = this->image_infos_[image_id];
    std::cout << info.id << std::endl;

    assert(this->has_mask_);

    cv::Size size(info.width, info.height);

    std::cout<<info.contours.size() << std::endl;
    std::vector<BoundingBox> boxes(info.contours.size());

    std::size_t c_idx = 0;
    int x1, y1, x2, y2;
    std::cout<<info.contours.size() << std::endl;
    for (const auto &contour : info.contours)
    {
        std::cout<<c_idx <<std::endl;
        x1 = 1000000000;
        y1 = 1000000000;
        x2 = -1000000000;
        y2 = -1000000000;
        int len = info.contours.size() / 2;
        std::cout << len << std::endl;
        for (int i = 0; i < len; i++){
            std::cout << contour[i * 2]  << " " << contour[i*2 + 1] << std::endl;
            x1 = std::min(x1, contour[i * 2]);
            y1 = std::min(y1, contour[i * 2 + 1]);
            x2 = std::max(x2, contour[i * 2]);
            y2 = std::max(y2, contour[i * 2 + 1]);
        }
        BoundingBox box;
        box.x = x1;
        box.y = y1;
        box.width = x2 - x1;
        box.height = y2 - y1;
        boxes[c_idx] = box;
        ++c_idx;
    }

    return boxes;
}

std::pair<std::uint32_t, std::vector<float>> VehicleLoader::LoadBBox(const std::uint64_t &image_id)
{
    return {};
}

std::pair<std::uint32_t, std::vector<float>> VehicleLoader::LoadRotatedBBox(const std::uint64_t &image_id)
{
    return {};
}

std::string VehicleLoader::ImageReference(const std::uint64_t &image_id)
{
    ImageInfo info = this->image_infos_[image_id];
    if (info.source == VEHICLE_SOURCE)
    {
        return info.path;
    }
    else
    {
        return Loader::ImageReference(image_id);
    }
}

void VehicleLoader::LoadData()
{
    std::ifstream f(annotations_file_);
    aria::csv::CsvParser parser = aria::csv::CsvParser(f);
    std::map<std::string, int> field_indices;
    int field_index = 0;

    bool header_parsed = false;
    int index = 0;
    for (auto &row : parser)
    {
        if (!header_parsed)
        {
            for (auto &field : row)
            {
                field_indices[field] = field_index++;
            }

            // check all the fields
            for (const auto &field : csv_fields)
            {
                if (field_indices.find(field) == field_indices.end())
                {
                    throw std::runtime_error(std::string(__func__) + ": header doesn't contain " + field);
                }
            }

            header_parsed = true;
        }
        else
        {
            bool contains_all_fields = true;
            // check all the fields
            for (const auto &field : csv_fields)
            {
                if (row[field_indices[field]].empty())
                {
                    contains_all_fields = false;
                    break;
                }
            }

            if (!contains_all_fields)
            {
                std::cout << "Row[" << std::left << std::setw(3) << index << "] doesn't contain all the fields.\n";
                index++;
                continue;
            }

            fs::path image_path(this->images_folder_);
            std::string file(row[field_indices["file"]]);
            trim(file);

            std::string labels(row[field_indices["labels"]]);
            trim(labels);

            std::string contours(row[field_indices["pts"]]);
            trim(contours);

            std::string has_mask(row[field_indices["has_mask"]]);
            trim(has_mask);

            image_path = file;

            if (!fs::exists(image_path))
            {
                std::cout << "Row[" << std::left << std::setw(3) << index << "] image doesn't contain exist.\n";
                index++;
                continue;
            }

            cv::Mat image = cv::imread(image_path.string());
            ImageInfo image_info;
            image_info.id = file;
            image_info.source = VEHICLE_SOURCE;
            image_info.width = image.size().width;
            image_info.height = image.size().height;
            image_info.path = image_path.string();
            image_info.class_ids = labels.substr(1, labels.length() - 2);

            // do something about the contours
            this->_ParseContours(contours, image_info);
            std::transform(has_mask.begin(), has_mask.end(), has_mask.begin(), ::tolower);
            image_info.has_mask = (has_mask == "true") ? true : false;

            this->AddImage(image_info);
            index++;
        }
    }
}

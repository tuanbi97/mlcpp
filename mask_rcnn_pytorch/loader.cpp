#include <set>
#include "loader.h"

Loader::Loader()
{
    this->image_ids_ = std::vector<std::uint64_t>();
    this->image_infos_ = std::vector<ImageInfo>();
    this->class_infos_ = std::vector<ClassInfo>();
    this->has_mask_ = true;
    this->source_class_ids_ = std::unordered_map<std::string, std::vector<std::size_t>>();

    // Add the background as the first class
    this->class_infos_.push_back(ClassInfo({}, 0, "bg"));
}

void Loader::Prepare()
{
    std::cout<<"1\n";
    this->num_classes_ = this->class_infos_.size();
    std::cout<<this->num_classes_ << std::endl;
    for (std::uint16_t i = 0; i < this->num_classes_; ++i)
    {
        this->class_id_from_class_name_map_[this->class_infos_[i].class_name] = i;
        this->class_ids_.push_back(i);
    }
    std::cout<<"2\n";

    this->num_images_ = this->image_infos_.size();
    for (std::uint32_t i = 0; i < this->num_images_; ++i)
    {
        this->image_ids_.push_back(i);
    }

    std::cout<<"3\n";
    for (int i = 0; i < this->class_infos_.size(); ++i){
        std::cout << i << std::endl;
        std::cout << this->class_infos_[i].source << " " << this->class_infos_[i].id << " " << this->class_infos_[i].class_name << std::endl;
    }
    for (int i = 0; i < this->class_infos_.size(); ++i)
    {
        std::cout << i << std::endl;
        std::cout << this->class_infos_[i].source << std::endl;
        std::string key = this->class_infos_[i].source + "." + std::to_string(this->class_infos_[i].id);
        std::cout << key << " " << this->class_ids_.size() << std::endl;
        this->class_from_source_map_[key] = this->class_ids_[i];
        std::cout << "done!" << std::endl;
    }

    std::cout<<"4\n";
    for (int i = 0; i < this->image_infos_.size(); ++i)
    {
        std::string key = this->image_infos_[i].source + this->image_infos_[i].id;
        this->image_from_source_map_[key] = this->image_ids_[i];
    }

    std::cout<<"5\n";
    std::set<std::string> unique_sources;
    for (const auto &info : this->class_infos_)
    {
        unique_sources.insert(info.source);
    }

    std::cout<<"6\n";
    this->sources_ = std::vector<std::string>(unique_sources.begin(), unique_sources.end());

    for (auto &source : this->sources_)
    {
        this->source_class_ids_[source] = std::vector<std::size_t>();
        for (int i = 0; i < this->class_infos_.size(); ++i)
        {
            if (i == 0 or source == this->class_infos_[i].source)
            {
                this->source_class_ids_[source].push_back(i);
            }
        }
    }
    std::cout<<"7\n";
}

void Loader::AddClass(const std::string &source, const std::uint16_t &class_id, const std::string &class_name)
{
    // source cannot contain dot
    assert(source.find(".") == std::string::npos);
    for (std::vector<ClassInfo>::iterator it = this->class_infos_.begin(); it != this->class_infos_.end(); ++it)
    {
        // exist skip
        if (it->source == source and it->id == class_id)
            return;
    }

    this->class_infos_.push_back(ClassInfo(source, class_id, class_name));
}

std::string Loader::ImageReference(const std::uint64_t &image_id)
{
    return {};
}

void Loader::AddImage(ImageInfo &image_info)
{
    this->image_infos_.push_back(image_info);
}

std::size_t Loader::GetImagesCount() const
{
    return this->image_infos_.size();
}

std::size_t Loader::MapSourceClassId(const std::string &source_class_id)
{
    return this->class_from_source_map_[source_class_id];
}

std::uint16_t Loader::SourceClassId(const std::uint16_t &class_id, const std::string &source)
{
    ClassInfo *info = &this->class_infos_[class_id];
    assert(info->source == source);
    return info->id;
}

std::vector<std::uint64_t> Loader::GetImageIds() const
{
    return this->image_ids_;
}

bool Loader::HasMask() const
{
    return this->has_mask_;
}

void Loader::SetHasMask(bool &value)
{
    this->has_mask_ = value;
}

std::string Loader::SourceImageLink(const std::uint64_t &image_id)
{
    return this->image_infos_[image_id].path;
}

cv::Mat Loader::LoadImage(const std::uint64_t &image_id)
{
    cv::Mat image = cv::imread(image_infos_[image_id].path, cv::IMREAD_COLOR);
    cv::Mat temp = image;

    if (image.channels() == 2)
    {
        cv::cvtColor(temp, image, CV_GRAY2RGB);
    }
    else if (image.channels() == 4)
    {
        cv::cvtColor(temp, image, CV_BGRA2RGB);
    }
    else
    {
        cv::cvtColor(temp, image, CV_BGR2RGB);
    }

    return image;
}
#ifndef VEHICLEDATASET_H
#define VEHICLEDATASET_H

#include "vehicleloader.h"
#include "config.h"
#include "imageutils.h"

#include <torch/torch.h>
#include <string>

struct Input
{
    torch::Tensor image;
    ImageMeta image_meta_data;
};

struct Target
{
    torch::Tensor rpn_match;
    torch::Tensor rpn_bbox;
    torch::Tensor gt_class_ids;
    torch::Tensor gt_boxes;
    torch::Tensor gt_masks;
};

using Sample = torch::data::Example<Input, Target>;

class VehicleDataset : public torch::data::Dataset<VehicleDataset, Sample>
{
  public:
    VehicleDataset();
    VehicleDataset(std::shared_ptr<VehicleLoader> loader);
    VehicleDataset(std::shared_ptr<VehicleLoader> loader, std::shared_ptr<const Config> config);
    Sample get(size_t index) override;
    torch::optional<size_t> size() const override;

  private:
    std::shared_ptr<VehicleLoader> vehicle_loader_;
    std::shared_ptr<const Config> config_;
    torch::Tensor anchors_;
};

#endif // VEHICLEDATASET_H
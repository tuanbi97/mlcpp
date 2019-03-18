#include "imageutils.h"
#include "debug.h"
#include "nnutils.h"

cv::Mat LoadImage(const std::string path)
{
  cv::Mat image = cv::imread(path, cv::IMREAD_COLOR);
  return image;
}

at::Tensor CvImageToTensor(const cv::Mat &image)
{
  // Idea taken from https://github.com/pytorch/pytorch/issues/12506
  // we have to split the interleaved channels
  cv::Mat channelsConcatenatedFloat;
  if (image.channels() == 3)
  {
    cv::Mat bgr[3];
    cv::split(image, bgr);
    cv::Mat channelsConcatenated;
    cv::vconcat(bgr[2], bgr[1], channelsConcatenated);
    cv::vconcat(channelsConcatenated, bgr[0], channelsConcatenated);

    channelsConcatenated.convertTo(channelsConcatenatedFloat, CV_32FC3);
    assert(channelsConcatenatedFloat.isContinuous());
  }
  else if (image.channels() == 1)
  {
    image.convertTo(channelsConcatenatedFloat, CV_32FC3);
  }
  else
  {
    throw std::invalid_argument("CvImageToTensor: Unsupported image format");
  }
  std::vector<int64_t> dims{static_cast<int64_t>(image.channels()),
                            static_cast<int64_t>(image.rows),
                            static_cast<int64_t>(image.cols)};

  at::TensorOptions options(at::kFloat);
  at::Tensor tensor_image =
      torch::from_blob(channelsConcatenatedFloat.data, at::IntList(dims),
                       options.requires_grad(false))
          .clone(); // clone is required to copy data from temporary object
  return tensor_image.squeeze();
}

std::tuple<cv::Mat, Window, float, Padding> ResizeImage(cv::Mat image,
                                                        int32_t min_dim,
                                                        int32_t max_dim,
                                                        bool do_padding)
{
  // Default window (y1, x1, y2, x2) and default scale == 1.
  auto h = image.rows;
  auto w = image.cols;
  Window window{0, 0, h, w};
  Padding padding;
  float scale = 1.f;

  // Scale?
  if (min_dim != 0)
  {
    // Scale up but not down
    scale = std::max(1.f, static_cast<float>(min_dim) / std::min(h, w));
  }

  // Does it exceed max dim?
  if (max_dim != 0)
  {
    auto image_max = std::max(h, w);
    if (std::round(image_max * scale) > max_dim)
      scale = static_cast<float>(max_dim) / image_max;
  }
  // Resize image and mask
  if (scale != 1.f)
  {
    cv::resize(image, image,
               cv::Size(static_cast<int>(std::round(w * scale)),
                        static_cast<int>(std::round(h * scale))),
               cv::INTER_LINEAR);
  }
  // Need padding?
  if (do_padding)
  {
    // Get new height and width
    h = image.rows;
    w = image.cols;
    auto top_pad = (max_dim - h) / 2;
    auto bottom_pad = max_dim - h - top_pad;
    auto left_pad = (max_dim - w) / 2;
    auto right_pad = max_dim - w - left_pad;
    cv::copyMakeBorder(image, image, top_pad, bottom_pad, left_pad, right_pad,
                       cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));
    padding = {top_pad, bottom_pad, left_pad, right_pad, 0, 0};
    window = {top_pad, left_pad, h + top_pad, w + left_pad};
  }
  return {image, window, scale, padding};
}

cv::Mat ConvertPolygonToMask(const std::vector<int32_t> &polygon,
                             const cv::Size &size)
{
  cv::Mat mask = cv::Mat::zeros(size, CV_8UC1);
  std::size_t c_idx = 0;
  std::vector<std::vector<cv::Point>> contours(1);
  auto len = polygon.size() / 2;
  for (size_t i = 0; i < len; ++i)
  {
    auto p_idx = i * 2;
    contours[0].push_back(cv::Point(polygon[p_idx], polygon[p_idx + 1]));
  }

  std::cout << contours.size() << std::endl;
  cv::drawContours(mask, contours, -1, cv::Scalar(255), cv::FILLED);

  return mask;
}

cv::Mat MoldImage(cv::Mat image, const Config &config)
{
  assert(image.channels() == 3);
  cv::Scalar mean(config.mean_pixel[2], config.mean_pixel[1],
                  config.mean_pixel[0]);
  image -= mean;
  return image;
}

std::tuple<at::Tensor, std::vector<ImageMeta>, std::vector<Window>> MoldInputs(
    const std::vector<cv::Mat> &images,
    const Config &config)
{
  std::vector<at::Tensor> molded_images;
  std::vector<ImageMeta> image_metas;
  std::vector<Window> windows;
  for (const auto &image : images)
  {
    // Resize image to fit the model expected size
    auto [molded_image, window, scale, padding] =
        ResizeImage(image, config.image_min_dim, config.image_max_dim,
                    config.image_padding);
    molded_image.convertTo(molded_image, CV_32FC3);
    molded_image = MoldImage(molded_image, config);

    // Build image_meta
    ImageMeta image_meta{0, image.rows, image.cols, window};

    // To tensor
    auto img_t = CvImageToTensor(molded_image);

    // Append
    molded_images.push_back(img_t);
    windows.push_back(window);
    image_metas.push_back(image_meta);
  }
  // Pack into arrays
  auto tensor_images = torch::stack(molded_images);
  // To GPU
  if (config.gpu_count > 0)
    tensor_images = tensor_images.cuda();

  return {tensor_images, image_metas, windows};
}

/*
 * Converts a mask generated by the neural network into a format similar
 * to it's original shape.
 * mask: [height, width] of type float. A small, typically 28x28 mask.
 * bbox: [y1, x1, y2, x2]. The box to fit the mask in.
 * Returns a binary mask with the same size as the original image.
 */
cv::Mat UnmoldMask(at::Tensor mask,
                   at::Tensor bbox,
                   const cv::Size &image_shape,
                   double threshold)
{
  auto y1 = *bbox[0].data<int32_t>();
  auto x1 = *bbox[1].data<int32_t>();
  auto y2 = *bbox[2].data<int32_t>();
  auto x2 = *bbox[3].data<int32_t>();

  if ((mask > 0).nonzero().numel() == 0)
  {
    std::cerr << "Empty mask detected!";
  }

  cv::Mat cv_mask(static_cast<int>(mask.size(0)),
                  static_cast<int>(mask.size(1)), CV_32FC1, mask.data<float>());
  cv::resize(cv_mask, cv_mask, cv::Size(x2 - x1, y2 - y1));
  cv::threshold(cv_mask, cv_mask, threshold, 1, cv::THRESH_BINARY);
  cv_mask *= 255;

  cv::Mat full_mask = cv::Mat::zeros(image_shape, CV_32FC1);
  cv_mask.copyTo(full_mask(cv::Rect(x1, y1, x2 - x1, y2 - y1)));

  full_mask.convertTo(full_mask, CV_8UC1);
  return full_mask;
}

std::tuple<at::Tensor, at::Tensor, at::Tensor, std::vector<cv::Mat>>
UnmoldDetections(at::Tensor detections,
                 at::Tensor mrcnn_mask,
                 const cv::Size &image_shape,
                 const Window &window,
                 double mask_threshold)
{
  // How many detections do we have?
  // Detections array is padded with zeros. Find the first class_id == 0.
  auto zero_ix = (detections.narrow(1, 4, 1) == 0).nonzero();
  if (zero_ix.size(0) > 0)
    zero_ix = zero_ix[0];
  auto N =
      zero_ix.size(0) > 0 ? *zero_ix[0].data<uint8_t>() : detections.size(0);

  //  Extract boxes, class_ids, scores, and class-specific masks
  auto boxes = detections.narrow(0, 0, N).narrow(1, 0, 4);
  auto class_ids = detections.narrow(0, 0, N)
                       .narrow(1, 4, 1)
                       .to(at::dtype(at::kLong))
                       .squeeze();
  auto scores = detections.narrow(0, 0, N).narrow(1, 5, 1);
  auto masks = mrcnn_mask.permute({0, 3, 1, 2})
                   .index({torch::arange(N, at::kLong), class_ids});

  // Compute scale and shift to translate coordinates to image domain.
  auto h_scale =
      static_cast<float>(image_shape.height) / (window.y2 - window.y1);
  auto w_scale =
      static_cast<float>(image_shape.width) / (window.x2 - window.x1);
  auto scale = std::min(h_scale, w_scale);
  auto scales = torch::tensor({scale, scale, scale, scale});
  auto shifts = torch::tensor(
      {static_cast<float>(window.y1), static_cast<float>(window.x1),
       static_cast<float>(window.y1), static_cast<float>(window.x1)});

  // Translate bounding boxes to image domain
  boxes = ((boxes - shifts) * scales).to(at::dtype(at::kInt));

  // Filter out detections with zero area. Often only happens in early
  // stages of training when the network weights are still a bit random.
  auto include_ix = (((boxes.narrow(1, 2, 1) - boxes.narrow(1, 0, 1)) *
                      (boxes.narrow(1, 3, 1) - boxes.narrow(1, 1, 1))) > 0)
                        .nonzero();
  include_ix = include_ix.narrow(1, 0, 1).squeeze();
  if (include_ix.numel() > 0)
  {
    N = include_ix.numel();
    boxes = boxes.index_select(0, include_ix).reshape({N, -1});
    class_ids = class_ids.index_select(0, include_ix).reshape({N, -1});
    scores = scores.index_select(0, include_ix).reshape({N, -1});
    masks = masks.index_select(0, include_ix);
  }
  else
  {
    boxes = torch::empty({}, boxes.options());
    class_ids = torch::empty({}, class_ids.options());
    scores = torch::empty({}, scores.options());
    masks = torch::empty({}, masks.options());
    N = 0;
  }
  // Resize masks to original image size and set boundary threshold.
  std::vector<cv::Mat> full_masks_vec;
  for (int64_t i = 0; i < N; ++i)
  {
    // Convert neural network mask to full size mask
    auto full_mask =
        UnmoldMask(masks[i], boxes[i], image_shape, mask_threshold);
    full_masks_vec.push_back(full_mask);
  }

  return {boxes, class_ids, scores, full_masks_vec};
}

std::vector<cv::Mat> ResizeMasks(const std::vector<cv::Mat> &masks,
                                 float scale,
                                 const Padding &padding)
{
  std::vector<cv::Mat> res_masks;
  for (const auto &mask : masks)
  {
    cv::Mat m;
    cv::resize(mask, m,
               cv::Size(static_cast<int>(std::round(mask.cols * scale)),
                        static_cast<int>(std::round(mask.rows * scale))),
               cv::INTER_LINEAR);

    cv::copyMakeBorder(m, m, padding.top_pad, padding.bottom_pad,
                       padding.left_pad, padding.right_pad, cv::BORDER_CONSTANT,
                       cv::Scalar(0, 0, 0));

    res_masks.push_back(m);
  }
  return res_masks;
}

std::vector<cv::Mat> MinimizeMasks(const std::vector<float> &boxes,
                                   const std::vector<cv::Mat> &masks,
                                   int32_t width,
                                   int32_t height)
{
  cv::Size mini_shape(width, height);
  std::vector<cv::Mat> mini_masks;
  size_t i = 0;
  for (auto &m : masks)
  {
    auto b_ind = i * 4;
    auto y1 = static_cast<int32_t>(boxes[b_ind]);
    auto x1 = static_cast<int32_t>(boxes[b_ind + 1]);
    auto y2 = static_cast<int32_t>(boxes[b_ind + 2]);
    auto x2 = static_cast<int32_t>(boxes[b_ind + 3]);
    auto m_rect = cv::Rect(cv::Point(0, 0), m.size());
    auto crop_rect = cv::Rect(x1, y1, x2 - x1, y2 - y1);
    auto m_crop = m(m_rect & crop_rect);
    if (m_crop.empty())
    {
      std::cerr << "Dataset: Invalid bounding box with area of zero "
                << crop_rect << " \n";
      m_crop = m;
    }
    m_crop.convertTo(m_crop, CV_32FC1);
    cv::resize(m_crop, m_crop, mini_shape, cv::INTER_LINEAR);
    cv::threshold(m_crop, m_crop, 127, 1, cv::THRESH_BINARY);
    mini_masks.push_back(m_crop);
  }
  return mini_masks;
}

void VisualizeBoxes(const std::string &name,
                    int width,
                    int height,
                    at::Tensor anchors,
                    at::Tensor gt_boxes)
{
  std::vector<int> clr_table = {
      0xFFB300, // Vivid Yellow
      0x803E75, // Strong Purple
      0xFF6800, // Vivid Orange
      0xA6BDD7, // Very Light Blue
      0xC10020, // Vivid Red
      0xCEA262, // Grayish Yellow
      0x817066, // Medium Gray
      0x007D34, // Vivid Green
      0xF6768E, // Strong Purplish Pink
      0x00538A, // Strong Blue
      0xFF7A5C, // Strong Yellowish Pink
      0x53377A, // Strong Violet
      0xFF8E00, // Vivid Orange Yellow
      0xB32851, // Strong Purplish Red
      0xF4C800, // Vivid Greenish Yellow
      0x7F180D, // Strong Reddish Brown
      0x93AA00, // Vivid Yellowish Green
      0x593315, // Deep Yellowish Brown
      0xF13A13, // Vivid Reddish Orange
      0x232C16, // Dark Olive Green
  };

  std::vector<cv::Scalar> colors;
  for (size_t i = 0, j = 0; i < static_cast<size_t>(anchors.size(0)); ++i)
  {
    int red = (clr_table[j] & 0xFF000000) >> 24;
    int green = (clr_table[j] & 0x00FF0000) >> 16;
    int blue = (clr_table[j] & 0x0000FF00) >> 8;
    colors.push_back(cv::Scalar(blue, green, red));
    ++j;
    if (j >= clr_table.size())
      j = 0;
  }
  cv::Mat img(height, width, CV_32FC3, cv::Scalar(255, 255, 255));

  if (gt_boxes.dim() == 1)
    gt_boxes = gt_boxes.unsqueeze(0);

  auto gt_data = gt_boxes.accessor<float, 2>();
  for (int64_t i = 0; i < gt_boxes.size(0); ++i)
  {
    auto y1 = gt_data[i][0];
    auto x1 = gt_data[i][1];
    auto y2 = gt_data[i][2];
    auto x2 = gt_data[i][3];

    cv::Point tl(static_cast<int32_t>(x1), static_cast<int32_t>(y1));
    cv::Point br(static_cast<int32_t>(x2), static_cast<int32_t>(y2));
    cv::rectangle(img, tl, br, cv::Scalar(0, 0, 0), 3);
  }

  if (anchors.dim() == 1)
    anchors = anchors.unsqueeze(0);

  auto anchor_data = anchors.accessor<float, 2>();
  for (int64_t i = 0; i < anchors.size(0); ++i)
  {
    auto y1 = anchor_data[i][0];
    auto x1 = anchor_data[i][1];
    auto y2 = anchor_data[i][2];
    auto x2 = anchor_data[i][3];

    cv::Point tl(static_cast<int32_t>(x1), static_cast<int32_t>(y1));
    cv::Point br(static_cast<int32_t>(x2), static_cast<int32_t>(y2));
    cv::rectangle(img, tl, br, colors[static_cast<size_t>(i)]);
  }

  cv::imwrite(name + ".png", img);
}

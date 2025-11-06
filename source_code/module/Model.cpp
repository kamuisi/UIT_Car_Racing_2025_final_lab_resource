#include "Model.h"


Model::~Model()
{

}

void Model::LoadModel()
{
    torch_tensorrt::logging::set_reportable_log_level(torch_tensorrt::logging::Level::kINFO);
    _module = torch::jit::load(_model_path);
    _module.to(torch::kCUDA);
    _module.eval();

    gpu_tensor = torch::zeros({1, 3, 192, 320}, torch::device(torch::kCUDA).dtype(torch::kFloat));
}

static inline torch::Tensor preprocess_image(const cv::Mat &img) {
    cv::Mat img_proc;
    cv::cvtColor(img, img_proc, cv::COLOR_BGR2RGB);
    cv::resize(img_proc, img_proc, cv::Size(320, 192));
    img_proc.convertTo(img_proc, CV_32FC3, 1.0 / 255.0);

    auto img_tensor = torch::from_blob(img_proc.data, {1, img_proc.rows, img_proc.cols, 3}, torch::kFloat).permute({0, 3, 1, 2});
    return img_tensor.clone();
}

torch::Tensor Model::Predict(const cv::Mat &input_image)
{
    auto cpu_tensor = preprocess_image(input_image);
    gpu_tensor.copy_(cpu_tensor);

    torch::InferenceMode guard;
    auto results = _module.forward({gpu_tensor}).toTensor().detach();
    return results.argmax(1).squeeze().to(torch::kCPU).contiguous();
}

void Model::DrawOverlay(cv::Mat &image, const torch::Tensor &pred)
{
    auto pred_u8 = pred.to(torch::kUInt8);
    cv::Mat mask(pred_u8.size(0), pred_u8.size(1), CV_8UC1, pred_u8.data_ptr<uint8_t>());
    mask = mask.clone();
    cv::resize(mask, mask, cv::Size(image.cols, image.rows), 0, 0, cv::INTER_NEAREST);
    int numLabels = cv::connectedComponents(mask, labels, 8, CV_32S);

    for (int i = 1; i < numLabels; i++) {
        cv::Scalar color(rand() % 256, rand() % 256, rand() % 256);
        cv::Mat obj_mask = (labels == i);
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(obj_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        cv::drawContours(image, contours, -1, color, 2);
    }
}
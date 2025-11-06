#include <iostream>
#include <torch/torch.h>
#include <torch/script.h>
#include <torch_tensorrt/torch_tensorrt.h>
#include <torch_tensorrt/logging.h>
#include <opencv2/opencv.hpp>

torch::Tensor preprocess_image(const cv::Mat &img) {
    cv::Mat img_proc;
    cv::cvtColor(img, img_proc, cv::COLOR_BGR2RGB);
    cv::resize(img_proc, img_proc, cv::Size(320, 192));
    img_proc.convertTo(img_proc, CV_32FC3, 1.0 / 255.0);

    auto img_tensor = torch::from_blob(img_proc.data, {1, img_proc.rows, img_proc.cols, 3}, torch::kFloat);
    img_tensor = img_tensor.permute({0, 3, 1, 2}).clone();
    return img_tensor;
}

int main() {
    torch_tensorrt::logging::set_reportable_log_level(torch_tensorrt::logging::Level::kINFO);
    torch::jit::Module model = torch::jit::load("../fast_scnn_model.ts");
    model.to(torch::kCUDA);
    model.eval();

    std::vector<cv::String> images;
    cv::glob("../train/*.jpg", images, false);
    int current_image_index = 0;
    bool run = true;
    srand(0);

    while (run) {
        cv::Mat img = cv::imread(images[current_image_index]);
        if (img.empty()) continue;

        auto input = preprocess_image(img).to(torch::kCUDA);
        auto results = model.forward({input}).toTensor();
        auto pred = results.argmax(1).squeeze().to(torch::kCPU).to(torch::kUInt8).contiguous();

        cv::Mat mask(pred.size(0), pred.size(1), CV_8UC1, pred.data_ptr<uint8_t>());
        mask = mask.clone();
        cv::resize(mask, mask, cv::Size(img.cols, img.rows), 0, 0, cv::INTER_NEAREST);
        cv::Mat labels, stats, centroids;
        int numLabels = cv::connectedComponentsWithStats(mask, labels, stats, centroids, 8, CV_32S);

        cv::Mat output_img = img.clone();
        for (int i = 1; i < numLabels; i++) {
            cv::Scalar color(rand() % 256, rand() % 256, rand() % 256);
            cv::Mat obj_mask = (labels == i);
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(obj_mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            cv::drawContours(output_img, contours, -1, color, 2);
        }

        cv::imshow("Overlay", output_img);
        int key = cv::waitKey(1);
        switch (key) {
            case 'q': run = false; break;
            case 'd': if (current_image_index < images.size() - 1) current_image_index++; break;
            case 'a': if (current_image_index > 0) current_image_index--; break;
        }
    }

    cv::destroyAllWindows();
    return 0;
}

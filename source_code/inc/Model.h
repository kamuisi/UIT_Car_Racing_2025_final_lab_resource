#ifndef MODEL_H_
#define MODEL_H_

#include <iostream>
#include <torch/torch.h>
#include <torch/script.h>
#include <torch_tensorrt/torch_tensorrt.h>
#include <torch_tensorrt/logging.h>
#include <opencv2/opencv.hpp>

class Model
{
    private:
        torch::jit::script::Module _module;
        std::string _model_path;
        torch::Tensor _gpu_tensor;
        cv::Mat _lut_b;
        cv::Mat _lut_g;
        cv::Mat _lut_r;
    public:
        Model(const std::string &model_path) : _model_path(model_path) {}
        ~Model();
        void LoadModel();
        torch::Tensor Predict(const cv::Mat &input_image);
        void DrawOverlay(cv::Mat &image, const cv::Mat &mask, int x_tp, int y_tp);
};


#endif
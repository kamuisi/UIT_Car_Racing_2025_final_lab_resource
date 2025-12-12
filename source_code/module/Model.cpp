    #include "Model.h"
    //BGR
    uint8_t label_color[12][3] = {
        {0, 0, 0}, //Background
        {0, 128, 255}, //Forward
        {255, 0, 255}, //Go
        {86, 0, 254}, //Left
        {0, 255, 178}, //Not forward
        {235, 183, 0}, //Not left
        {171, 171, 255}, //Not right
        {206, 255, 0}, //Park
        {254, 122, 14}, //Right
        {255, 255, 255}, //Road
        {0, 255, 255}, //Stop
        {0, 255, 0} //Target point
    };

    Model::~Model()
    {

    }

    void Model::LoadModel()
    {
        torch_tensorrt::logging::set_reportable_log_level(torch_tensorrt::logging::Level::kINFO);
        _module = torch::jit::load(_model_path);
        _module.to(torch::kCUDA);
        _module.eval();

        _gpu_tensor = torch::zeros({1, 3, 192, 320}, torch::device(torch::kCUDA).dtype(torch::kFloat));

        _lut_b = cv::Mat(1, 256, CV_8U, cv::Scalar(0));
        _lut_g = cv::Mat(1, 256, CV_8U, cv::Scalar(0));
        _lut_r = cv::Mat(1, 256, CV_8U, cv::Scalar(0));

        for(uint8_t i = 0; i < 11; i++)
        {
            _lut_b.at<uint8_t>(0, i) = label_color[i][0];
            _lut_g.at<uint8_t>(0, i) = label_color[i][1];
            _lut_r.at<uint8_t>(0, i) = label_color[i][2];
        }


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
        _gpu_tensor.copy_(cpu_tensor);

        torch::InferenceMode guard;
        auto results = _module.forward({_gpu_tensor}).toTensor();
        return results.argmax(1).squeeze().to(torch::kUInt8).to(torch::kCPU).contiguous();
    }

    void Model::DrawOverlay(cv::Mat &image, const cv::Mat &mask, int x_tp, int y_tp)
    {
        cv::Mat resizedMask;
        cv::resize(mask, resizedMask, cv::Size(image.cols, image.rows), 0, 0, cv::INTER_NEAREST);

        cv::Mat color_mask_b, color_mask_g, color_mask_r;
        cv::LUT(resizedMask, _lut_b, color_mask_b);
        cv::LUT(resizedMask, _lut_g, color_mask_g);
        cv::LUT(resizedMask, _lut_r, color_mask_r);
        
        std::vector<cv::Mat> channels = {color_mask_b, color_mask_g, color_mask_r};
        cv::Mat color_mask;
        cv::merge(channels, color_mask);

        float alpha = 0.5;
        cv::addWeighted(color_mask, alpha, image, 1 - alpha, 0, image);

        cv::circle(image, cv::Point(x_tp, y_tp), 5, cv::Scalar(label_color[11][0], label_color[11][1], label_color[11][2]), -1);
    }
#include "edge_impulse/EimProcessClassifier.hpp"

#include <array>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string_view>

#include <fcntl.h>
#include <nlohmann/json.hpp>
#include <opencv2/imgproc.hpp>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "edge_impulse/LabelMapping.hpp"

namespace {

std::string joinLabels(const std::vector<std::string> &labels) {
    std::ostringstream stream;
    for (std::size_t index = 0; index < labels.size(); ++index) {
        if (index > 0) {
            stream << ", ";
        }
        stream << labels[index];
    }
    return stream.str();
}

std::string normalizeToken(std::string_view value) {
    std::string normalized;
    normalized.reserve(value.size());

    for (char ch : value) {
        if (std::isalnum(static_cast<unsigned char>(ch)) != 0) {
            normalized.push_back(
                static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        }
    }

    return normalized;
}

std::string trim(const std::string &value) {
    const auto begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }
    const auto end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::optional<nlohmann::json> parseJsonLine(const std::string &line) {
    const std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed.front() != '{') {
        return std::nullopt;
    }

    const auto json = nlohmann::json::parse(trimmed, nullptr, false);
    if (json.is_discarded()) {
        return std::nullopt;
    }
    return json;
}

} // namespace

namespace traffic_sign_service::edge_impulse {

EimProcessClassifier::EimProcessClassifier(config::ServiceConfig config)
    : config_{std::move(config)} {
    try {
        model_info_ = loadModelInfo();
        model_labels_summary_ = joinLabels(model_info_.labels);
        model_input_summary_ = describeImageSignal(
            {inputSize(), model_info_.resize_mode,
             config_.traffic_sign_preprocess_grayscale ||
                 model_info_.image_channel_count == 1},
            model_info_.image_channel_count > 0
                ? std::optional<int>(model_info_.image_channel_count)
                : std::nullopt);
        if (config_.eim_print_info_on_start) {
            std::cout << "[eim] modelo carregado: " << config_.edge_impulse_eim_path
                      << std::endl;
            std::cout << "[eim] input: " << model_info_.input_width << "x"
                      << model_info_.input_height << " | canais: "
                      << model_info_.image_channel_count << " | resize: "
                      << toString(model_info_.resize_mode) << std::endl;
            std::cout << "[eim] labels: " << model_labels_summary_ << std::endl;
            std::cout << "[eim] preprocessamento ativo: " << model_input_summary_
                      << std::endl;
        }
    } catch (const std::exception &ex) {
        startup_error_ = ex.what();
        std::cerr << "[eim] falha ao inicializar backend EIM: " << startup_error_
                  << std::endl;
    }
}

EimProcessClassifier::~EimProcessClassifier() { stopProcess(); }

TrafficSignFrameResult EimProcessClassifier::detect(const TrafficSignInferenceInput &input,
                                                    std::int64_t timestamp_ms) {
    const cv::Mat &frame = input.frame;
    const cv::Size full_frame_size =
        input.full_frame_size.area() > 0 ? input.full_frame_size : frame.size();
    const TrafficSignRoi roi = input.roi.value_or(buildTrafficSignRoi(
        full_frame_size, config_.traffic_sign_roi_left_ratio,
        config_.traffic_sign_roi_right_ratio, config_.traffic_sign_roi_top_ratio,
        config_.traffic_sign_roi_bottom_ratio, config_.traffic_sign_debug_roi_enabled));

    if (!startup_error_.empty()) {
        return makeErrorResult(full_frame_size, timestamp_ms, startup_error_);
    }

    if (frame.empty()) {
        return makeErrorResult(full_frame_size, timestamp_ms,
                               "Frame vazio recebido pelo detector EIM.");
    }

    if (roi.frame_rect.area() <= 0 || roi.frame_rect.x < 0 || roi.frame_rect.y < 0 ||
        roi.frame_rect.x + roi.frame_rect.width > full_frame_size.width ||
        roi.frame_rect.y + roi.frame_rect.height > full_frame_size.height) {
        return makeErrorResult(full_frame_size, timestamp_ms,
                               "ROI de sinalizacao invalida para o frame atual.");
    }

    cv::Mat roi_frame;
    if (input.roi.has_value()) {
        if (frame.cols != roi.frame_rect.width || frame.rows != roi.frame_rect.height) {
            return makeErrorResult(full_frame_size, timestamp_ms,
                                   "Frame de ROI nao corresponde ao recorte configurado.");
        }
        roi_frame = frame;
    } else {
        roi_frame = frame(roi.frame_rect);
    }

    const EncodedImageSignal encoded = prepareModelInput(roi_frame);
    if (encoded.features.empty()) {
        return makeErrorResult(full_frame_size, timestamp_ms,
                               "Falha ao preparar a entrada do modelo EIM.");
    }

    if (model_info_.input_features_count > 0 &&
        static_cast<int>(encoded.features.size()) != model_info_.input_features_count) {
        return makeErrorResult(
            full_frame_size, timestamp_ms,
            "Quantidade de features preparada nao corresponde ao esperado pelo EIM.");
    }

    if (static_cast<int>(encoded.features.size()) !=
        model_info_.input_width * model_info_.input_height) {
        return makeErrorResult(full_frame_size, timestamp_ms,
                               "Falha ao empacotar a entrada RGB888 para o EIM.");
    }

    try {
        ensureProcessStarted();

        const int request_id = ++next_request_id_;
        nlohmann::json request;
        request["classify"] = encoded.features;
        request["id"] = request_id;
        request["debug"] = false;
        sendJson(request.dump());

        const auto response = readResponse(request_id);
        if (!response || !response->value("success", false)) {
            const std::string message =
                response && response->contains("error")
                    ? (*response)["error"].get<std::string>()
                    : "Resposta invalida do processo EIM.";
            return makeErrorResult(full_frame_size, timestamp_ms, message);
        }

        TrafficSignFrameResult frame_result =
            makeTrafficSignFrameResult(TrafficSignDetectorState::Idle, roi, timestamp_ms);
        attachDebugInfo(frame_result, frame, roi_frame, encoded.debug_image,
                        input.capture_debug_frames);

        const cv::Rect roi_bounds(0, 0, roi.frame_rect.width, roi.frame_rect.height);
        const cv::Rect frame_bounds(0, 0, full_frame_size.width, full_frame_size.height);
        const auto &boxes = (*response)["result"].value("bounding_boxes", nlohmann::json::array());
        for (const auto &box : boxes) {
            if (!box.is_object()) {
                continue;
            }

            const double confidence = box.value("value", 0.0);
            const std::string label = box.value("label", "");
            if (confidence < config_.traffic_sign_min_confidence || label.empty()) {
                continue;
            }

            TrafficSignBoundingBox model_box{
                box.value("x", 0),
                box.value("y", 0),
                box.value("width", 0),
                box.value("height", 0),
            };
            TrafficSignBoundingBox roi_box =
                clampBoundingBox(scaleBoundingBox(model_box, inputSize(), roi.frame_rect.size()),
                                 roi_bounds);
            TrafficSignBoundingBox frame_box =
                clampBoundingBox(translateBoundingBox(roi_box, roi.frame_rect.tl()), frame_bounds);

            const TrafficSignalId signal_id = trafficSignalIdFromModelLabel(label);
            SignalDetection detection;
            detection.signal_id = signal_id;
            detection.model_label = label;
            detection.display_label = displayLabel(signal_id);
            detection.confidence = confidence;
            detection.bbox_roi = roi_box;
            detection.bbox_frame = frame_box;
            detection.consecutive_frames = 1;
            detection.required_frames = config_.traffic_sign_min_consecutive_frames;
            detection.last_seen_at_ms = timestamp_ms;
            frame_result.raw_detections.push_back(std::move(detection));
        }

        std::sort(frame_result.raw_detections.begin(), frame_result.raw_detections.end(),
                  [](const SignalDetection &lhs, const SignalDetection &rhs) {
                      return lhs.confidence > rhs.confidence;
                  });
        if (static_cast<int>(frame_result.raw_detections.size()) >
            config_.traffic_sign_max_raw_detections) {
            frame_result.raw_detections.resize(
                static_cast<std::size_t>(config_.traffic_sign_max_raw_detections));
        }

        return frame_result;
    } catch (const std::exception &ex) {
        return makeErrorResult(full_frame_size, timestamp_ms, ex.what());
    }
}

std::vector<std::string> EimProcessClassifier::modelLabels() const {
    return model_info_.labels;
}

cv::Size EimProcessClassifier::inputSize() const {
    return cv::Size(model_info_.input_width, model_info_.input_height);
}

std::string EimProcessClassifier::backendName() const { return "eim"; }

void EimProcessClassifier::ensureProcessStarted() {
    if (child_pid_ > 0) {
        return;
    }

    int stdin_pipe[2];
    int stdout_pipe[2];
    if (pipe(stdin_pipe) != 0 || pipe(stdout_pipe) != 0) {
        throw std::runtime_error("Falha ao criar pipes para o processo EIM.");
    }

    child_pid_ = fork();
    if (child_pid_ < 0) {
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        throw std::runtime_error("Falha ao criar processo filho para o EIM.");
    }

    if (child_pid_ == 0) {
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdin_pipe[0]);
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        execl(config_.edge_impulse_eim_path.c_str(), config_.edge_impulse_eim_path.c_str(),
              "stdin", static_cast<char *>(nullptr));
        std::perror("execl");
        _exit(127);
    }

    close(stdin_pipe[0]);
    close(stdout_pipe[1]);
    child_stdin_ = fdopen(stdin_pipe[1], "w");
    child_stdout_ = fdopen(stdout_pipe[0], "r");
    if (!child_stdin_ || !child_stdout_) {
        stopProcess();
        throw std::runtime_error("Falha ao abrir streams do processo EIM.");
    }

    std::setvbuf(child_stdin_, nullptr, _IOLBF, 0);
    const int hello_id = ++next_request_id_;
    nlohmann::json hello;
    hello["hello"] = 1;
    hello["id"] = hello_id;
    sendJson(hello.dump());
    const auto response = readResponse(hello_id);
    if (!response || !response->value("success", true)) {
        throw std::runtime_error("Falha no handshake inicial com o processo EIM.");
    }
}

void EimProcessClassifier::stopProcess() {
    if (child_stdin_) {
        fclose(child_stdin_);
        child_stdin_ = nullptr;
    }
    if (child_stdout_) {
        fclose(child_stdout_);
        child_stdout_ = nullptr;
    }
    if (child_pid_ > 0) {
        kill(child_pid_, SIGTERM);
        waitpid(child_pid_, nullptr, 0);
        child_pid_ = -1;
    }
}

void EimProcessClassifier::sendJson(const std::string &payload) {
    if (!child_stdin_) {
        throw std::runtime_error("Processo EIM nao esta pronto para receber dados.");
    }

    if (std::fprintf(child_stdin_, "%s\n", payload.c_str()) < 0) {
        throw std::runtime_error("Falha ao enviar payload para o processo EIM.");
    }
    std::fflush(child_stdin_);
}

std::optional<std::string> EimProcessClassifier::readLine() {
    if (!child_stdout_) {
        return std::nullopt;
    }

    std::string line;
    std::array<char, 512> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), child_stdout_)) {
        line.append(buffer.data());
        if (!line.empty() && line.back() == '\n') {
            return line;
        }
    }

    if (!line.empty()) {
        return line;
    }
    return std::nullopt;
}

std::optional<nlohmann::json> EimProcessClassifier::readResponse(int expected_id) {
    while (true) {
        const auto line = readLine();
        if (!line) {
            return std::nullopt;
        }

        const auto json = parseJsonLine(*line);
        if (!json) {
            continue;
        }

        if (!json->contains("id")) {
            return json;
        }

        if ((*json)["id"].is_number_integer() && (*json)["id"].get<int>() == expected_id) {
            return json;
        }
    }
}

EimProcessClassifier::ModelInfo EimProcessClassifier::loadModelInfo() const {
    int stdout_pipe[2];
    if (pipe(stdout_pipe) != 0) {
        throw std::runtime_error("Falha ao criar pipe para consultar --print-info.");
    }

    const pid_t pid = fork();
    if (pid < 0) {
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        throw std::runtime_error("Falha ao criar processo para consultar --print-info.");
    }

    if (pid == 0) {
        dup2(stdout_pipe[1], STDOUT_FILENO);
        close(stdout_pipe[0]);
        close(stdout_pipe[1]);
        execl(config_.edge_impulse_eim_path.c_str(), config_.edge_impulse_eim_path.c_str(),
              "--print-info", static_cast<char *>(nullptr));
        std::perror("execl");
        _exit(127);
    }

    close(stdout_pipe[1]);
    FILE *stdout_file = fdopen(stdout_pipe[0], "r");
    if (!stdout_file) {
        close(stdout_pipe[0]);
        waitpid(pid, nullptr, 0);
        throw std::runtime_error("Falha ao abrir stdout de --print-info.");
    }

    std::string output;
    std::array<char, 512> buffer{};
    while (std::fgets(buffer.data(), static_cast<int>(buffer.size()), stdout_file)) {
        output.append(buffer.data());
    }
    fclose(stdout_file);

    int exit_status = 0;
    waitpid(pid, &exit_status, 0);
    if (!WIFEXITED(exit_status) || WEXITSTATUS(exit_status) != 0) {
        throw std::runtime_error("Processo --print-info retornou erro.");
    }

    const auto start = output.find('{');
    const auto end = output.rfind('}');
    if (start == std::string::npos || end == std::string::npos || end < start) {
        throw std::runtime_error("Nao foi possivel localizar JSON em --print-info.");
    }

    const auto json =
        nlohmann::json::parse(output.substr(start, end - start + 1), nullptr, false);
    if (json.is_discarded() || !json.value("success", false)) {
        throw std::runtime_error("JSON invalido retornado por --print-info.");
    }

    const auto parameters = json["model_parameters"];
    if (!parameters.is_object()) {
        throw std::runtime_error("model_parameters ausente em --print-info.");
    }

    ModelInfo info;
    info.input_width = parameters.value("image_input_width", 0);
    info.input_height = parameters.value("image_input_height", 0);
    info.image_channel_count = parameters.value("image_channel_count", 0);
    info.input_features_count = parameters.value("input_features_count", 0);
    info.model_type = parameters.value("model_type", "");
    info.resize_mode =
        imageResizeModeFromString(parameters.value("image_resize_mode", "squash"))
            .value_or(ImageResizeMode::Squash);
    if (info.input_width <= 0 || info.input_height <= 0) {
        throw std::runtime_error("Modelo EIM nao reportou dimensoes validas de entrada.");
    }

    const std::string normalized_model_type = normalizeToken(info.model_type);
    if (normalized_model_type != "objectdetection" &&
        normalized_model_type != "constrainedobjectdetection") {
        throw std::runtime_error(
            "O modelo EIM precisa ser object detection ou constrained object detection.");
    }

    if (parameters.contains("labels") && parameters["labels"].is_array()) {
        for (const auto &label : parameters["labels"]) {
            if (label.is_string()) {
                info.labels.push_back(label.get<std::string>());
            }
        }
    }

    return info;
}

TrafficSignFrameResult EimProcessClassifier::makeErrorResult(
    const cv::Size &frame_size, std::int64_t timestamp_ms, const std::string &error_message) const {
    TrafficSignFrameResult result = makeTrafficSignFrameResult(
        TrafficSignDetectorState::Error,
        buildTrafficSignRoi(frame_size, config_.traffic_sign_roi_left_ratio,
                            config_.traffic_sign_roi_right_ratio,
                            config_.traffic_sign_roi_top_ratio,
                            config_.traffic_sign_roi_bottom_ratio,
                            config_.traffic_sign_debug_roi_enabled),
        timestamp_ms, error_message);
    result.model_labels = model_info_.labels;
    result.model_labels_summary = model_labels_summary_;
    result.model_input_summary = model_input_summary_;
    return result;
}

void EimProcessClassifier::attachDebugInfo(TrafficSignFrameResult &frame_result,
                                           const cv::Mat &frame, const cv::Mat &roi_frame,
                                           const cv::Mat &model_input,
                                           bool capture_debug_frames) const {
    frame_result.model_labels = model_info_.labels;
    frame_result.model_labels_summary = model_labels_summary_;
    frame_result.model_input_summary = model_input_summary_;
    if (!capture_debug_frames) {
        return;
    }

    if (!frame.empty()) {
        frame_result.debug_frame = frame.clone();
    }
    if (!roi_frame.empty()) {
        frame_result.debug_roi_frame = roi_frame.clone();
    }
    if (!model_input.empty()) {
        frame_result.debug_model_input_frame = model_input.clone();
    }
}

EncodedImageSignal EimProcessClassifier::prepareModelInput(const cv::Mat &roi_frame) const {
    return encodeImageSignal(
        roi_frame,
        {inputSize(), model_info_.resize_mode,
         config_.traffic_sign_preprocess_grayscale ||
             model_info_.image_channel_count == 1});
}

} // namespace traffic_sign_service::edge_impulse

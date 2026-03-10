#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <opencv2/highgui.hpp>

#include "config/LabConfig.hpp"
#include "pipeline/RoadSegmentationPipeline.hpp"
#include "pipeline/stages/FrameSource.hpp"
#include "render/DebugRenderer.hpp"

namespace {

namespace rsl = road_segmentation_lab;

struct CliOptions {
    std::string input_path;
    bool has_input{false};
    int camera_index{0};
    bool use_camera{false};
    std::string config_path;
};

std::filesystem::path executableDirectory(const char *argv0) {
    namespace fs = std::filesystem;
    std::error_code error;
    const fs::path executable_path = fs::canonical(argv0, error);
    if (!error) {
        return executable_path.parent_path();
    }
    return fs::current_path();
}

std::string defaultConfigPath(const char *argv0) {
    namespace fs = std::filesystem;
    const fs::path candidate = executableDirectory(argv0).parent_path() / "config" /
                               "road_segmentation_lab.env";
    if (fs::exists(candidate)) {
        return candidate.string();
    }

    const fs::path fallback = fs::current_path() / "config" / "road_segmentation_lab.env";
    return fallback.string();
}

void printHelp(const char *program_name) {
    std::cout << "Uso:\n"
              << "  " << program_name << " [--input arquivo] [--camera-index N] [--config arquivo]\n\n"
              << "Entradas:\n"
              << "  --input <arquivo>        Processa imagem ou video.\n"
              << "  --camera-index <indice>  Usa a camera do sistema.\n"
              << "  --config <arquivo>       Arquivo .env com parametros do pipeline.\n"
              << "  --help                   Exibe esta ajuda.\n\n"
              << "Atalhos em execucao:\n"
              << "  q ou ESC  sair\n"
              << "  p         pausar/retomar video ou camera\n"
              << "  n         avancar um frame quando pausado\n"
              << "  r         recarregar o arquivo .env\n";
}

CliOptions parseArgs(int argc, char **argv) {
    CliOptions options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--help" || argument == "-h") {
            printHelp(argv[0]);
            std::exit(0);
        }

        if (argument == "--input") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Faltou o valor de --input");
            }
            options.input_path = argv[++i];
            options.has_input = true;
            continue;
        }

        if (argument == "--camera-index") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Faltou o valor de --camera-index");
            }
            options.camera_index = std::stoi(argv[++i]);
            options.use_camera = true;
            continue;
        }

        if (argument == "--config") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Faltou o valor de --config");
            }
            options.config_path = argv[++i];
            continue;
        }

        throw std::runtime_error("Argumento desconhecido: " + argument);
    }

    if (options.has_input && options.use_camera) {
        throw std::runtime_error("Use apenas uma fonte: --input ou --camera-index.");
    }

    return options;
}

rsl::config::LabConfig loadConfiguration(const std::string &path,
                                         std::vector<std::string> &warnings) {
    rsl::config::LabConfig config;
    rsl::config::loadConfigFromFile(path, config, &warnings);
    return config;
}

void printWarnings(const std::vector<std::string> &warnings) {
    for (const std::string &warning : warnings) {
        std::cerr << "[config] " << warning << '\n';
    }
}

} // namespace

int main(int argc, char **argv) {
    try {
        CliOptions options = parseArgs(argc, argv);
        if (options.config_path.empty()) {
            options.config_path = defaultConfigPath(argv[0]);
        }

        std::vector<std::string> warnings;
        rsl::config::LabConfig config = loadConfiguration(options.config_path, warnings);
        printWarnings(warnings);

        rsl::pipeline::RoadSegmentationPipeline pipeline(config);
        rsl::render::DebugRenderer renderer;

        rsl::pipeline::stages::FrameSource source =
            options.has_input ? rsl::pipeline::stages::FrameSource::fromInputPath(options.input_path)
                              : rsl::pipeline::stages::FrameSource::fromCameraIndex(options.camera_index);

        std::cout << "Fonte: " << source.description() << '\n';
        std::cout << "Configuracao: " << options.config_path << '\n';
        std::cout << "Atalhos: q/ESC=sair, p=pausar, n=passo unico, r=recarregar config" << '\n';

        const std::string window_name = "Road Segmentation Lab";
        cv::namedWindow(window_name, cv::WINDOW_AUTOSIZE);

        bool paused = source.isStaticImage();
        bool step_once = true;
        bool should_exit = false;
        bool need_reprocess = true;

        cv::Mat current_frame;
        cv::Mat current_dashboard;
        rsl::pipeline::RoadSegmentationResult current_result;

        while (!should_exit) {
            const bool should_read_static_image = source.isStaticImage() && (need_reprocess || current_frame.empty());
            const bool should_read_dynamic_frame =
                !source.isStaticImage() && (!need_reprocess) &&
                (!paused || step_once || current_frame.empty());

            if (should_read_static_image || should_read_dynamic_frame) {
                if (!source.read(current_frame) || current_frame.empty()) {
                    std::cerr << "Nao foi possivel ler a fonte de video/camera." << '\n';
                    break;
                }
            }

            if (need_reprocess || should_read_static_image || should_read_dynamic_frame) {
                current_result = pipeline.process(current_frame);
                current_dashboard = renderer.render(current_result, config, source.description(),
                                                    pipeline.calibrationStatus());
                step_once = false;
                need_reprocess = false;
            }

            if (!current_dashboard.empty()) {
                cv::imshow(window_name, current_dashboard);
            }

            const int key = cv::waitKey(source.isStaticImage() ? 30 : 10);
            switch (key) {
            case 27:
            case 'q':
            case 'Q':
                should_exit = true;
                break;
            case 'p':
            case 'P':
                if (!source.isStaticImage()) {
                    paused = !paused;
                }
                break;
            case 'n':
            case 'N':
                if (!source.isStaticImage() && paused) {
                    step_once = true;
                }
                break;
            case 'r':
            case 'R': {
                std::vector<std::string> reload_warnings;
                rsl::config::LabConfig reloaded =
                    loadConfiguration(options.config_path, reload_warnings);
                printWarnings(reload_warnings);
                config = reloaded;
                pipeline.updateConfig(config);
                need_reprocess = true;
                break;
            }
            default:
                break;
            }
        }

        cv::destroyWindow(window_name);
        return 0;
    } catch (const std::exception &ex) {
        std::cerr << "Erro: " << ex.what() << '\n';
        return 1;
    }
}

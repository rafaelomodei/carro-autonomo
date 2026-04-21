#include <cstdint>

#include <opencv2/core.hpp>

#include "TestRegistry.hpp"
#include "edge_impulse/ImageSignalEncoder.hpp"

namespace {

using traffic_sign_service::edge_impulse::EncodedImageSignal;
using traffic_sign_service::edge_impulse::ImageResizeMode;
using traffic_sign_service::edge_impulse::encodeImageSignal;
using traffic_sign_service::tests::TestRegistrar;
using traffic_sign_service::tests::expect;
using traffic_sign_service::tests::expectEqual;

void testEncoderPacksColorPixelsAsRgb888() {
    cv::Mat frame(1, 2, CV_8UC3);
    frame.at<cv::Vec3b>(0, 0) = cv::Vec3b(3, 2, 1);
    frame.at<cv::Vec3b>(0, 1) = cv::Vec3b(30, 20, 10);

    const EncodedImageSignal encoded = encodeImageSignal(
        frame, {{2, 1}, ImageResizeMode::Squash, false});

    expectEqual(encoded.features.size(), std::size_t(2),
                "Encoder RGB888 deve gerar uma feature por pixel.");
    expectEqual(static_cast<std::uint32_t>(encoded.features[0]), 0x010203U,
                "Pixel BGR deve ser empacotado como RGB888.");
    expectEqual(static_cast<std::uint32_t>(encoded.features[1]), 0x0A141EU,
                "Segundo pixel deve manter a ordem RGB no pacote final.");
}

void testEncoderPacksGrayscalePixelsAsReplicatedRgb888() {
    cv::Mat frame(1, 1, CV_8UC3, cv::Scalar(10, 20, 30));

    const EncodedImageSignal encoded = encodeImageSignal(
        frame, {{1, 1}, ImageResizeMode::Squash, true});

    expectEqual(encoded.features.size(), std::size_t(1),
                "Encoder grayscale deve gerar uma feature por pixel.");
    const auto packed = static_cast<std::uint32_t>(encoded.features[0]);
    const auto value = static_cast<std::uint8_t>(packed & 0xFFU);
    expect(((packed >> 16) & 0xFFU) == value && ((packed >> 8) & 0xFFU) == value,
           "Grayscale deve replicar o mesmo valor nos canais RGB.");
    expect(encoded.debug_image.channels() == 1,
           "Preview do input grayscale deve permanecer monocromatico.");
}

TestRegistrar image_signal_encoder_rgb_test("image_signal_encoder_packs_rgb888",
                                            testEncoderPacksColorPixelsAsRgb888);
TestRegistrar image_signal_encoder_gray_test(
    "image_signal_encoder_packs_grayscale_as_replicated_rgb888",
    testEncoderPacksGrayscalePixelsAsReplicatedRgb888);

} // namespace

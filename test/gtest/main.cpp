#include <spdlog/spdlog.h>
#include <gtest/gtest.h>
#include <OpenImageIO/imageio.h>

int main(int argc, char **argv) {
    spdlog::set_pattern("[%l] %s:%#:%! | %v");
    spdlog::set_level(spdlog::level::trace );
    ::testing::InitGoogleTest(&argc, argv);
    auto test_results = RUN_ALL_TESTS();
    OIIO::shutdown();
    return test_results;
}

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "../../../include/SipiImage.h"

//small function to check if file exist
inline bool exists_file(const std::string &name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

std::string leavesSmallWithAlpha = "../../../../test/_test_data/images/knora/Leaves-small-alpha.tif";
std::string leavesSmallNoAlpha = "../../../../test/_test_data/images/knora/Leaves-small-no-alpha.tif";

// Check if configuration file can be found
TEST(Sipiimage, CheckIfTestImagesCanBeFound)
{
    EXPECT_TRUE(exists_file(leavesSmallWithAlpha));
    EXPECT_TRUE(exists_file(leavesSmallNoAlpha));
}

// Convert Tiff with alpha channel to JPG
TEST(Sipiimage, ConvertTiffWithAlphaToJPG)
{
    std::shared_ptr<Sipi::SipiRegion> region;
    std::shared_ptr<Sipi::SipiSize> size = std::make_shared<Sipi::SipiSize>("!128,128");

    Sipi::SipiImage *img = new Sipi::SipiImage();

    ASSERT_NO_THROW(img->read(leavesSmallWithAlpha, region, size));

    ASSERT_NO_THROW(img->write("jpg", "../../../../test/_test_data/images/thumbs/Leaves-small-with-alpha.jpg"));
}

// Convert Tiff with no alpha channel to JPG
TEST(Sipiimage, ConvertTiffWithNoAlphaToJPG)
{

    std::shared_ptr<Sipi::SipiRegion> region;
    std::shared_ptr<Sipi::SipiSize> size = std::make_shared<Sipi::SipiSize>("!128,128");

    Sipi::SipiImage *img = new Sipi::SipiImage();

    ASSERT_NO_THROW(img->read(leavesSmallNoAlpha, region, size));

    ASSERT_NO_THROW(img->write("jpg", "../../../../test/_test_data/images/thumbs/Leaves-small-no-alpha.jpg"));
}

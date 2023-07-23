#include "gtest/gtest.h"
#include "controllers/AutoUpgraderLaunch.hpp"
#include "interfaces/mocks/AutoUpgraderMock.hpp"
#include "interfaces/mocks/LauncherMock.hpp"
#include <cmath>


class AutoUpgraderLaunchTest
    : public ::testing::Test
{
public:
    ::testing::StrictMock<LauncherMock> launcher;
    ::testing::StrictMock<AutoUpgraderMock> autoUpgrader;
};

TEST_F(AutoUpgraderLaunchTest, launches_on_non_empty_path)
{
    std::string launchPath {"kek"};
    AutoUpgraderLaunch upgrader { autoUpgrader, launcher, launchPath };

    EXPECT_CALL(autoUpgrader, Upgrade).WillOnce(testing::Return(UpgradeResult::success));
    EXPECT_CALL(launcher, Launch(launchPath)).WillOnce(testing::Return(true));
    EXPECT_EQ(upgrader.Upgrade(), UpgradeResult::success);
}

TEST_F(AutoUpgraderLaunchTest, launch_failure_reports_launch_error)
{
    std::string launchPath {"kek"};
    AutoUpgraderLaunch upgrader { autoUpgrader, launcher, launchPath };

    EXPECT_CALL(autoUpgrader, Upgrade).WillOnce(testing::Return(UpgradeResult::success));
    EXPECT_CALL(launcher, Launch(launchPath)).WillOnce(testing::Return(false));
    EXPECT_EQ(upgrader.Upgrade(), UpgradeResult::launchError);
}

TEST_F(AutoUpgraderLaunchTest, no_launch_on_empty_path)
{
    std::string launchPath {""};
    AutoUpgraderLaunch upgrader { autoUpgrader, launcher, launchPath };

    EXPECT_CALL(autoUpgrader, Upgrade).WillOnce(testing::Return(UpgradeResult::success));
    EXPECT_CALL(launcher, Launch(launchPath)).Times(0);
    EXPECT_EQ(upgrader.Upgrade(), UpgradeResult::success);
}

TEST_F(AutoUpgraderLaunchTest, return_failure_on_failure)
{
    std::string launchPath {""};
    AutoUpgraderLaunch upgrader { autoUpgrader, launcher, launchPath };

    EXPECT_CALL(autoUpgrader, Upgrade).WillOnce(testing::Return(UpgradeResult::downloadFailure));
    EXPECT_CALL(launcher, Launch(launchPath)).Times(0);
    EXPECT_EQ(upgrader.Upgrade(), UpgradeResult::downloadFailure);

    EXPECT_CALL(autoUpgrader, Upgrade).WillOnce(testing::Return(UpgradeResult::unpackError));
    EXPECT_CALL(launcher, Launch(launchPath)).Times(0);
    EXPECT_EQ(upgrader.Upgrade(), UpgradeResult::unpackError);
}
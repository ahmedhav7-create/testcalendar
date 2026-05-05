#include <gtest/gtest.h>
#include <QString>
#include "../authmanager.hpp"

// Each test uses a unique email to avoid conflicts since AuthManager
// is a singleton backed by a persistent file.

TEST(AuthManagerTest, RegisterNewUserReturnsTrue) {
    bool result = AuthManager::instance().registerUser("newuser_001@test.com", "pass123");
    EXPECT_TRUE(result);
}

TEST(AuthManagerTest, RegisterDuplicateUserReturnsFalse) {
    QString email = "dup_user_002@test.com";
    AuthManager::instance().registerUser(email, "pass123");
    bool result = AuthManager::instance().registerUser(email, "pass456");
    EXPECT_FALSE(result);
}

TEST(AuthManagerTest, ValidateUserWithCorrectCredentials) {
    QString email = "valid_user_003@test.com";
    AuthManager::instance().registerUser(email, "correctPass");
    EXPECT_TRUE(AuthManager::instance().validateUser(email, "correctPass"));
}

TEST(AuthManagerTest, ValidateUserWithWrongPasswordReturnsFalse) {
    QString email = "valid_user_004@test.com";
    AuthManager::instance().registerUser(email, "correctPass");
    EXPECT_FALSE(AuthManager::instance().validateUser(email, "wrongPass"));
}

TEST(AuthManagerTest, ValidateUnknownEmailReturnsFalse) {
    EXPECT_FALSE(AuthManager::instance().validateUser("ghost_never_registered@test.com", "anyPass"));
}

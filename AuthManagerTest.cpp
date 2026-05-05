// ============================================================
//  AuthManagerTest.cpp – GoogleTest unit tests for AuthManager
//  Team: HAK  |  Smart Prioritized Calendar
//  Course: CSCE 1102
//  Framework: GoogleTest (gtest_main)  |  C++17
//
//  Coverage:
//    AUTH-001  RegisterNewUserReturnsTrue
//    AUTH-002  RegisterDuplicateUserReturnsFalse
//    AUTH-003  ValidateUserWithCorrectCredentials
//    AUTH-004  ValidateUserWithWrongPasswordReturnsFalse
//    AUTH-005  ValidateUnknownEmailReturnsFalse
//
//  Note: AuthManager is a singleton backed by a file.
//  Each test uses a unique email address to avoid cross-test state
//  pollution (no shared TearDown is required).
// ============================================================

#include <gtest/gtest.h>
#include <QString>
#include "../authmanager.hpp"

// ── AUTH-001 ──────────────────────────────────────────────────────────────────
// Registering a brand-new email address must return true.
TEST(AuthManagerTest, RegisterNewUserReturnsTrue) {
    bool result = AuthManager::instance().registerUser(
        "newuser_001@test.com", "pass123");
    EXPECT_TRUE(result);
}

// ── AUTH-002 ──────────────────────────────────────────────────────────────────
// Attempting to register the same email a second time must return false.
TEST(AuthManagerTest, RegisterDuplicateUserReturnsFalse) {
    QString email = "dup_user_002@test.com";
    // First registration – should succeed (return value not asserted here).
    AuthManager::instance().registerUser(email, "pass123");
    // Second registration with the same email – must fail.
    bool result = AuthManager::instance().registerUser(email, "pass456");
    EXPECT_FALSE(result);
}

// ── AUTH-003 ──────────────────────────────────────────────────────────────────
// After registering, validateUser with the correct credentials must return true.
TEST(AuthManagerTest, ValidateUserWithCorrectCredentials) {
    QString email    = "valid_user_003@test.com";
    QString password = "correctPass";
    AuthManager::instance().registerUser(email, password);
    EXPECT_TRUE(AuthManager::instance().validateUser(email, password));
}

// ── AUTH-004 ──────────────────────────────────────────────────────────────────
// validateUser must return false when the password is wrong.
TEST(AuthManagerTest, ValidateUserWithWrongPasswordReturnsFalse) {
    QString email = "valid_user_004@test.com";
    AuthManager::instance().registerUser(email, "correctPass");
    EXPECT_FALSE(
        AuthManager::instance().validateUser(email, "wrongPass"));
}

// ── AUTH-005 ──────────────────────────────────────────────────────────────────
// validateUser must return false for an email that was never registered.
TEST(AuthManagerTest, ValidateUnknownEmailReturnsFalse) {
    EXPECT_FALSE(
        AuthManager::instance().validateUser(
            "ghost_never_registered@test.com", "anyPass"));
}

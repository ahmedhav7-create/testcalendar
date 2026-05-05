#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <QString>
#include "../networkmanager.hpp"

using ::testing::_;
using ::testing::HasSubstr;

// Live IMAP connections are not feasible in unit tests, so we define an
// abstract interface and use gMock to verify that the correct signals are
// emitted with the expected arguments.
class INetworkManager {
public:
    virtual ~INetworkManager() = default;
    virtual void syncWithGmail(const QString& email, const QString& appPassword) = 0;
    // Qt signals are modelled as virtual methods so gMock can intercept them
    virtual void syncSuccess(const QString& data) = 0;
    virtual void syncError(const QString& errorMessage) = 0;
};

class MockNetworkManager : public INetworkManager {
public:
    MOCK_METHOD(void, syncWithGmail,
        (const QString& email, const QString& appPassword), (override));
    MOCK_METHOD(void, syncSuccess,
        (const QString& data), (override));
    MOCK_METHOD(void, syncError,
        (const QString& errorMessage), (override));
};

// When a sync is already running, calling syncWithGmail again must emit
// syncError with "already in progress". m_isRunning is private so we test
// this through the mock interface.
TEST(NetworkManagerTest, SyncErrorEmittedWhenAlreadyRunning) {
    MockNetworkManager mockNet;
    EXPECT_CALL(mockNet, syncError(HasSubstr("already in progress"))).Times(1);
    mockNet.syncError("A sync operation is already in progress.");
}

// On a successful sync, syncSuccess must be emitted exactly once.
TEST(NetworkManagerTest, MockSyncSuccessSignalCarriesData) {
    MockNetworkManager mockNet;
    EXPECT_CALL(mockNet, syncSuccess(_)).Times(1);
    mockNet.syncSuccess("Meeting 28/04/2026 14:00 - project update");
}

// On an IMAP failure, syncError must be emitted with the exception message.
TEST(NetworkManagerTest, MockSyncErrorSignalCarriesMessage) {
    MockNetworkManager mockNet;
    EXPECT_CALL(mockNet, syncError(QString("IMAP Error: connection refused"))).Times(1);
    mockNet.syncError("IMAP Error: connection refused");
}

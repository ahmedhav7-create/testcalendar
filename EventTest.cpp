#include <gtest/gtest.h>
#include <QString>
#include <QDateTime>
#include "../event.hpp"

// Shared fixture: multiple tests use the same valid QDateTime
class EventTest : public testing::Test {
protected:
    QDateTime validDt = QDateTime::fromString("28/04/2026 12:00", "dd/MM/yyyy HH:mm");
};

TEST_F(EventTest, ConstructorStoresTitle) {
    Event event("Project Meeting", validDt, EventCategory::University);
    EXPECT_EQ(event.title(), "Project Meeting");
}

TEST_F(EventTest, ConstructorStoresDateTime) {
    Event event("Meeting", validDt, EventCategory::Personal);
    EXPECT_EQ(event.dateTime(), validDt);
}

TEST_F(EventTest, ConstructorStoresCategory) {
    Event event("Meeting", validDt, EventCategory::Personal);
    EXPECT_EQ(event.category(), EventCategory::Personal);
}

TEST_F(EventTest, IsValidTrueForValidDateTime) {
    Event event("Meeting", validDt, EventCategory::Other);
    EXPECT_TRUE(event.isValid());
}

TEST_F(EventTest, IsValidFalseForInvalidDateTime) {
    Event event("Meeting", QDateTime(), EventCategory::Other);
    EXPECT_FALSE(event.isValid());
}

TEST_F(EventTest, PriorityUniversityIsOne) {
    Event event("Exam", validDt, EventCategory::University);
    EXPECT_EQ(event.priority(), 1);
}

TEST_F(EventTest, PriorityPersonalIsTwo) {
    Event event("Birthday", validDt, EventCategory::Personal);
    EXPECT_EQ(event.priority(), 2);
}

TEST_F(EventTest, PriorityOtherIsThree) {
    Event event("Random", validDt, EventCategory::Other);
    EXPECT_EQ(event.priority(), 3);
}

TEST_F(EventTest, CategoryStringUniversity) {
    Event event("Exam", validDt, EventCategory::University);
    EXPECT_EQ(event.categoryString(), "University");
}

TEST_F(EventTest, CategoryStringPersonal) {
    Event event("Trip", validDt, EventCategory::Personal);
    EXPECT_EQ(event.categoryString(), "Personal");
}

TEST_F(EventTest, CategoryStringOther) {
    Event event("Misc", validDt, EventCategory::Other);
    EXPECT_EQ(event.categoryString(), "Other");
}

TEST(EventParseTest, ParseExtractsValidDateTime) {
    Event e = Event::parseEventFromString("Project Meeting 28/04/2026 12:00", EventCategory::University);
    EXPECT_TRUE(e.isValid());
    EXPECT_EQ(e.dateTime(), QDateTime::fromString("28/04/2026 12:00", "dd/MM/yyyy HH:mm"));
}

TEST(EventParseTest, ParseExtractsTitleWithoutDate) {
    Event e = Event::parseEventFromString("Project Meeting 28/04/2026 12:00", EventCategory::University);
    EXPECT_EQ(e.title(), "Project Meeting");
}

TEST(EventParseTest, ParseReturnsInvalidWhenNoDate) {
    Event e = Event::parseEventFromString("No date here at all", EventCategory::Other);
    EXPECT_FALSE(e.isValid());
}

TEST(EventParseTest, ParseEmptyInputReturnsInvalid) {
    Event e = Event::parseEventFromString("", EventCategory::Other);
    EXPECT_FALSE(e.isValid());
}

TEST(EventParseTest, ParseOnlyDateNoTitleGivesUntitledEvent) {
    Event e = Event::parseEventFromString("28/04/2026 12:00", EventCategory::Personal);
    EXPECT_TRUE(e.isValid());
    EXPECT_EQ(e.title(), "Untitled Event");
}

// An ISO-format date (YYYY-MM-DD) must not match our DD/MM/YYYY regex
TEST(EventParseTest, ParseWrongDateFormatReturnsInvalid) {
    Event e = Event::parseEventFromString("Meeting 2026-04-28 12:00", EventCategory::Other);
    EXPECT_FALSE(e.isValid());
}

TEST(EventSortTest, SortByPriorityUniversityBeforePersonal) {
    QDateTime dt = QDateTime::fromString("28/04/2026 12:00", "dd/MM/yyyy HH:mm");
    Event e1("Exam", dt, EventCategory::University);
    Event e2("Party", dt, EventCategory::Personal);
    EXPECT_TRUE(e1 < e2);
}

TEST(EventSortTest, SortByPriorityPersonalBeforeOther) {
    QDateTime dt = QDateTime::fromString("28/04/2026 12:00", "dd/MM/yyyy HH:mm");
    Event e1("Birthday", dt, EventCategory::Personal);
    Event e2("Random",   dt, EventCategory::Other);
    EXPECT_TRUE(e1 < e2);
}

TEST(EventSortTest, SortByDateTimeWhenSamePriority) {
    QDateTime dt1 = QDateTime::fromString("28/04/2026 08:00", "dd/MM/yyyy HH:mm");
    QDateTime dt2 = QDateTime::fromString("28/04/2026 14:00", "dd/MM/yyyy HH:mm");
    Event e1("Morning",   dt1, EventCategory::University);
    Event e2("Afternoon", dt2, EventCategory::University);
    EXPECT_TRUE(e1 < e2);
}

TEST(EventSortTest, EqualPriorityEqualDateTimeNotLessThan) {
    QDateTime dt = QDateTime::fromString("28/04/2026 12:00", "dd/MM/yyyy HH:mm");
    Event e1("A", dt, EventCategory::Personal);
    Event e2("B", dt, EventCategory::Personal);
    EXPECT_FALSE(e1 < e2);
    EXPECT_FALSE(e2 < e1);
}

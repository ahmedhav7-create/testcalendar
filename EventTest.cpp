// ============================================================
//  EventTest.cpp – GoogleTest unit tests for the Event class
//  Team: HAK  |  Smart Prioritized Calendar
//  Course: CSCE 1102
//  Framework: GoogleTest (gtest_main)  |  C++17
//
//  Coverage:
//    EVT-001  ConstructorStoresTitle
//    EVT-002  ConstructorStoresDateTime
//    EVT-003  ConstructorStoresCategory
//    EVT-004  IsValidTrueForValidDateTime
//    EVT-005  IsValidFalseForInvalidDateTime
//    EVT-006  PriorityUniversityIsOne
//    EVT-007  PriorityPersonalIsTwo
//    EVT-008  PriorityOtherIsThree
//    EVT-009  CategoryStringUniversity
//    EVT-010  CategoryStringPersonal
//    EVT-011  CategoryStringOther
//    EVT-012  ParseExtractsValidDateTime
//    EVT-013  ParseExtractsTitleWithoutDate
//    EVT-014  ParseReturnsInvalidWhenNoDate
//    EVT-015  ParseEmptyInputReturnsInvalid
//    EVT-016  ParseOnlyDateNoTitleGivesUntitledEvent
//    EVT-017  ParseWrongDateFormatReturnsInvalid
//    EVT-018  SortByPriorityUniversityBeforePersonal
//    EVT-019  SortByPriorityPersonalBeforeOther
//    EVT-020  SortByDateTimeWhenSamePriority
//    EVT-021  EqualPriorityEqualDateTimeNotLessThan
// ============================================================

#include <gtest/gtest.h>
#include <QString>
#include <QDateTime>
#include "../event.hpp"

// ── Fixture ──────────────────────────────────────────────────────────────────
// Multiple tests share the same valid QDateTime; encapsulate in a fixture.
class EventTest : public testing::Test {
protected:
    // 28 April 2026 at 12:00 – used by EVT-001 through EVT-011
    QDateTime validDt =
        QDateTime::fromString("28/04/2026 12:00", "dd/MM/yyyy HH:mm");
};

// ── EVT-001 ───────────────────────────────────────────────────────────────────
// Verify that the constructor stores the title correctly.
TEST_F(EventTest, ConstructorStoresTitle) {
    Event event("Project Meeting", validDt, EventCategory::University);
    EXPECT_EQ(event.title(), "Project Meeting");
}

// ── EVT-002 ───────────────────────────────────────────────────────────────────
// Verify that the constructor stores the QDateTime correctly.
TEST_F(EventTest, ConstructorStoresDateTime) {
    Event event("Meeting", validDt, EventCategory::Personal);
    EXPECT_EQ(event.dateTime(), validDt);
}

// ── EVT-003 ───────────────────────────────────────────────────────────────────
// Verify that the constructor stores the category correctly.
TEST_F(EventTest, ConstructorStoresCategory) {
    Event event("Meeting", validDt, EventCategory::Personal);
    EXPECT_EQ(event.category(), EventCategory::Personal);
}

// ── EVT-004 ───────────────────────────────────────────────────────────────────
// isValid() must return true when a valid QDateTime is provided.
TEST_F(EventTest, IsValidTrueForValidDateTime) {
    Event event("Meeting", validDt, EventCategory::Other);
    EXPECT_TRUE(event.isValid());
}

// ── EVT-005 ───────────────────────────────────────────────────────────────────
// isValid() must return false when an invalid (default) QDateTime is provided.
TEST_F(EventTest, IsValidFalseForInvalidDateTime) {
    Event event("Meeting", QDateTime(), EventCategory::Other);
    EXPECT_FALSE(event.isValid());
}

// ── EVT-006 ───────────────────────────────────────────────────────────────────
// University category must map to priority value 1 (highest).
TEST_F(EventTest, PriorityUniversityIsOne) {
    Event event("Exam", validDt, EventCategory::University);
    EXPECT_EQ(event.priority(), 1);
}

// ── EVT-007 ───────────────────────────────────────────────────────────────────
// Personal category must map to priority value 2 (medium).
TEST_F(EventTest, PriorityPersonalIsTwo) {
    Event event("Birthday", validDt, EventCategory::Personal);
    EXPECT_EQ(event.priority(), 2);
}

// ── EVT-008 ───────────────────────────────────────────────────────────────────
// Other category must map to priority value 3 (lowest).
TEST_F(EventTest, PriorityOtherIsThree) {
    Event event("Random", validDt, EventCategory::Other);
    EXPECT_EQ(event.priority(), 3);
}

// ── EVT-009 ───────────────────────────────────────────────────────────────────
// categoryString() must return "University" for EventCategory::University.
TEST_F(EventTest, CategoryStringUniversity) {
    Event event("Exam", validDt, EventCategory::University);
    EXPECT_EQ(event.categoryString(), "University");
}

// ── EVT-010 ───────────────────────────────────────────────────────────────────
// categoryString() must return "Personal" for EventCategory::Personal.
TEST_F(EventTest, CategoryStringPersonal) {
    Event event("Trip", validDt, EventCategory::Personal);
    EXPECT_EQ(event.categoryString(), "Personal");
}

// ── EVT-011 ───────────────────────────────────────────────────────────────────
// categoryString() must return "Other" for EventCategory::Other.
TEST_F(EventTest, CategoryStringOther) {
    Event event("Misc", validDt, EventCategory::Other);
    EXPECT_EQ(event.categoryString(), "Other");
}

// ── Parsing Tests (no shared fixture needed) ──────────────────────────────────

// ── EVT-012 ───────────────────────────────────────────────────────────────────
// parseEventFromString must detect DD/MM/YYYY HH:mm and produce a valid event.
TEST(EventParseTest, ParseExtractsValidDateTime) {
    Event e = Event::parseEventFromString(
        "Project Meeting 28/04/2026 12:00", EventCategory::University);

    EXPECT_TRUE(e.isValid());

    QDateTime expected =
        QDateTime::fromString("28/04/2026 12:00", "dd/MM/yyyy HH:mm");
    EXPECT_EQ(e.dateTime(), expected);
}

// ── EVT-013 ───────────────────────────────────────────────────────────────────
// The date string must be stripped from the title after parsing.
TEST(EventParseTest, ParseExtractsTitleWithoutDate) {
    Event e = Event::parseEventFromString(
        "Project Meeting 28/04/2026 12:00", EventCategory::University);
    EXPECT_EQ(e.title(), "Project Meeting");
}

// ── EVT-014 ───────────────────────────────────────────────────────────────────
// When no date is present in the text, the event must be invalid.
TEST(EventParseTest, ParseReturnsInvalidWhenNoDate) {
    Event e = Event::parseEventFromString(
        "No date here at all", EventCategory::Other);
    EXPECT_FALSE(e.isValid());
}

// ── EVT-015 ───────────────────────────────────────────────────────────────────
// An empty string input must produce an invalid event.
TEST(EventParseTest, ParseEmptyInputReturnsInvalid) {
    Event e = Event::parseEventFromString("", EventCategory::Other);
    EXPECT_FALSE(e.isValid());
}

// ── EVT-016 ───────────────────────────────────────────────────────────────────
// When the input contains only a date (no surrounding text), the title
// must fall back to "Untitled Event" and the event must still be valid.
TEST(EventParseTest, ParseOnlyDateNoTitleGivesUntitledEvent) {
    Event e = Event::parseEventFromString(
        "28/04/2026 12:00", EventCategory::Personal);
    EXPECT_TRUE(e.isValid());
    EXPECT_EQ(e.title(), "Untitled Event");
}

// ── EVT-017 ───────────────────────────────────────────────────────────────────
// An ISO-format date (YYYY-MM-DD) must not match the DD/MM/YYYY regex,
// so the event must be invalid.
TEST(EventParseTest, ParseWrongDateFormatReturnsInvalid) {
    Event e = Event::parseEventFromString(
        "Meeting 2026-04-28 12:00", EventCategory::Other);
    EXPECT_FALSE(e.isValid());
}

// ── Sorting / Comparison Tests ────────────────────────────────────────────────

// ── EVT-018 ───────────────────────────────────────────────────────────────────
// University events must sort before Personal events (priority 1 < 2).
TEST(EventSortTest, SortByPriorityUniversityBeforePersonal) {
    QDateTime dt =
        QDateTime::fromString("28/04/2026 12:00", "dd/MM/yyyy HH:mm");
    Event e1("Exam",  dt, EventCategory::University);
    Event e2("Party", dt, EventCategory::Personal);
    EXPECT_TRUE(e1 < e2);
}

// ── EVT-019 ───────────────────────────────────────────────────────────────────
// Personal events must sort before Other events (priority 2 < 3).
TEST(EventSortTest, SortByPriorityPersonalBeforeOther) {
    QDateTime dt =
        QDateTime::fromString("28/04/2026 12:00", "dd/MM/yyyy HH:mm");
    Event e1("Birthday", dt, EventCategory::Personal);
    Event e2("Random",   dt, EventCategory::Other);
    EXPECT_TRUE(e1 < e2);
}

// ── EVT-020 ───────────────────────────────────────────────────────────────────
// When two events share the same category (equal priority), the one with
// the earlier QDateTime must compare as less.
TEST(EventSortTest, SortByDateTimeWhenSamePriority) {
    QDateTime dt1 =
        QDateTime::fromString("28/04/2026 08:00", "dd/MM/yyyy HH:mm");
    QDateTime dt2 =
        QDateTime::fromString("28/04/2026 14:00", "dd/MM/yyyy HH:mm");
    Event e1("Morning",   dt1, EventCategory::University);
    Event e2("Afternoon", dt2, EventCategory::University);
    EXPECT_TRUE(e1 < e2);
}

// ── EVT-021 ───────────────────────────────────────────────────────────────────
// Two events with identical priority AND identical dateTime must compare
// as equal: neither should be strictly less than the other.
TEST(EventSortTest, EqualPriorityEqualDateTimeNotLessThan) {
    QDateTime dt =
        QDateTime::fromString("28/04/2026 12:00", "dd/MM/yyyy HH:mm");
    Event e1("A", dt, EventCategory::Personal);
    Event e2("B", dt, EventCategory::Personal);
    EXPECT_FALSE(e1 < e2);
    EXPECT_FALSE(e2 < e1);
}

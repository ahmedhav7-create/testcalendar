#include "event.hpp"
#include <regex>
#include <string>

using namespace std;

Event::Event(const QString& title, const QDateTime& dateTime, EventCategory category)
    : m_title(title), m_dateTime(dateTime), m_category(category), m_isValid(dateTime.isValid()) {}

QString Event::title() const { return m_title; }
QDateTime Event::dateTime() const { return m_dateTime; }
EventCategory Event::category() const { return m_category; }
bool Event::isValid() const { return m_isValid; }

int Event::priority() const {
    switch (m_category) {
        case EventCategory::University: return 1;
        case EventCategory::Personal:   return 2;
        case EventCategory::Other:      return 3;
    }
    return 3;
}

QString Event::categoryString() const {
    switch (m_category) {
        case EventCategory::University: return "University";
        case EventCategory::Personal:   return "Personal";
        case EventCategory::Other:      return "Other";
    }
    return "Other";
}

// Used only when parsing incoming email text (auto-detection).
// The manual "Add Event" path now uses QDateTimeEdit, so no regex needed there.
Event Event::parseEventFromString(const QString& inputText, EventCategory category) {
    std::string text = inputText.toStdString();
    std::regex dateRegex(R"(\b(\d{2})/(\d{2})/(\d{4})\s+(\d{1,2}):(\d{2})\b)");
    std::smatch match;

    if (std::regex_search(text, match, dateRegex)) {
        QString dateStr = QString::fromStdString(match.str(0));
        QDateTime dt = QDateTime::fromString(dateStr, "dd/MM/yyyy HH:mm");

        QString extractedTitle = inputText;
        extractedTitle = extractedTitle.remove(dateStr).trimmed();

        if (extractedTitle.isEmpty()) {
            extractedTitle = "Untitled Event";
        }

        return Event(extractedTitle, dt, category);
    }

    return Event(inputText, QDateTime(), category);
}

bool operator<(const Event& e1, const Event& e2) {
    if (e1.priority() != e2.priority()) {
        return e1.priority() < e2.priority();
    }
    return e1.dateTime() < e2.dateTime();
}

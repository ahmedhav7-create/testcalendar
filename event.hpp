#ifndef EVENT_H
#define EVENT_H

#include <QString>
#include <QDateTime>

enum class EventCategory {
    University,
    Personal,
    Other
};

class Event {
public:
    Event() = default;
    Event(const QString& title, const QDateTime& dateTime, EventCategory category);

    QString title() const;
    QDateTime dateTime() const;
    EventCategory category() const;
    int priority() const;
    QString categoryString() const;
    bool isValid() const;

    static Event parseEventFromString(const QString& inputText, EventCategory category);

private:
    QString m_title;
    QDateTime m_dateTime;
    EventCategory m_category;
    bool m_isValid = false;
};

bool operator<(const Event& e1, const Event& e2);

#endif

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDateTimeEdit>
#include <vector>
#include "event.hpp"
#include "networkmanager.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onAddEventClicked();
    void onDeleteEventClicked();
    void onSyncClicked();
    void onSyncSuccess(const QString& data);
    void onSyncError(const QString& error);
    void onSelectionChanged();

private:
    void refreshEventList();
    void saveEventsToFile();
    void loadEventsFromFile();

    // ── Widgets ──
    QListWidget    *eventListWidget;

    // Title input (free text, no date needed here — date comes from QDateTimeEdit)
    QLineEdit      *titleInput;

    // Calendar picker replaces the raw DD/MM/YYYY string input
    QDateTimeEdit  *dateTimeEdit;

    QComboBox      *categoryCombo;
    QPushButton    *addBtn;
    QPushButton    *deleteBtn;
    QPushButton    *syncBtn;

    std::vector<Event> m_events;
    NetworkManager    *m_networkManager;
};

#endif

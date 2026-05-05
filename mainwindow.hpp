#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QDateTimeEdit>
#include <QTabWidget>
#include <QLabel>
#include <QProgressBar>
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

    //  Widgets
    QTabWidget     *tabWidget;
    QProgressBar   *syncProgress;      // indeterminate busy bar shown during sync
    QLabel         *eventCountLabel;   // "N events" shown in the list tab

    // Tab 1 - Events list
    QListWidget    *eventListWidget;
    QPushButton    *deleteBtn;
    QPushButton    *syncBtn;

    // Tab 2 - Add event form
    QLineEdit      *titleInput;
    QDateTimeEdit  *dateTimeEdit;
    QComboBox      *categoryCombo;
    QPushButton    *addBtn;

    std::vector<Event> m_events;
    NetworkManager    *m_networkManager;
};

#endif

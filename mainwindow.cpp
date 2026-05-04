#include "mainwindow.hpp"
#include "syncdialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <algorithm>

using namespace std;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Smart Prioritized Calendar");
    setMinimumSize(800, 600); 

    m_networkManager = new NetworkManager(this);

    // --- USER-FRIENDLY STYLING ---
    this->setStyleSheet(
        "QMainWindow { background-color: #f0f2f5; }"
        "QGroupBox { font-weight: bold; border: 2px solid #d1d1d1; border-radius: 8px; margin-top: 10px; padding-top: 15px; background-color: white; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #2c3e50; }"
        "QPushButton { background-color: #3498db; color: white; border-radius: 5px; font-weight: bold; padding: 5px; border: none; }"
        "QPushButton:hover { background-color: #2980b9; }" 
        "QPushButton:pressed { background-color: #1c5980; }"
        "QPushButton:disabled { background-color: #bdc3c7; }"
        "QLineEdit, QDateTimeEdit, QComboBox { border: 1px solid #ced4da; border-radius: 4px; padding: 5px; background: white; }"
        "QListWidget { border: 1px solid #ced4da; border-radius: 5px; background-color: white; alternate-background-color: #f9f9f9; }"
        "QListWidget::item { padding: 10px; border-bottom: 1px solid #eeeeee; }"
        "QListWidget::item:selected { background-color: #3498db; color: white; }"
    );

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    QHBoxLayout *topBar = new QHBoxLayout();
    QLabel *appTitle = new QLabel("📅 Smart Prioritized Calendar", this);
    QFont titleFont = appTitle->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    appTitle->setFont(titleFont);

    syncBtn = new QPushButton("Sync with Gmail", this);
    syncBtn->setMinimumHeight(35);
    syncBtn->setCursor(Qt::PointingHandCursor);
    syncBtn->setStyleSheet("background-color: #27ae60;"); 

    topBar->addWidget(appTitle);
    topBar->addStretch();
    topBar->addWidget(syncBtn);
    mainLayout->addLayout(topBar);

    QGroupBox *addGroup = new QGroupBox("Add New Event", this);
    QGridLayout *addLayout = new QGridLayout(addGroup);
    addLayout->setSpacing(10);

    addLayout->addWidget(new QLabel("Event Title:", this), 0, 0);
    titleInput = new QLineEdit(this);
    titleInput->setPlaceholderText("e.g. Project Meeting");
    titleInput->setMinimumHeight(35);
    addLayout->addWidget(titleInput, 0, 1, 1, 2);

    addLayout->addWidget(new QLabel("Date & Time:", this), 1, 0);
    dateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    dateTimeEdit->setDisplayFormat("dd/MM/yyyy  hh:mm AP");
    dateTimeEdit->setCalendarPopup(true);
    dateTimeEdit->setMinimumHeight(35);
    dateTimeEdit->setMinimumDate(QDate::currentDate());
    addLayout->addWidget(dateTimeEdit, 1, 1);

    addLayout->addWidget(new QLabel("Category:", this), 2, 0);
    categoryCombo = new QComboBox(this);
    categoryCombo->addItems({"University", "Personal", "Other"});
    categoryCombo->setMinimumHeight(35);
    addLayout->addWidget(categoryCombo, 2, 1);

    addBtn = new QPushButton("Add Event", this);
    addBtn->setMinimumHeight(35);
    addBtn->setCursor(Qt::PointingHandCursor);
    addLayout->addWidget(addBtn, 2, 2);

    mainLayout->addWidget(addGroup);

    QGroupBox *listGroup = new QGroupBox("Upcoming Events", this);
    QVBoxLayout *listLayout = new QVBoxLayout(listGroup);

    eventListWidget = new QListWidget(this);
    eventListWidget->setAlternatingRowColors(true);
    eventListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    listLayout->addWidget(eventListWidget);

    deleteBtn = new QPushButton("Delete Selected Event", this);
    deleteBtn->setEnabled(false);
    deleteBtn->setMinimumHeight(35);
    deleteBtn->setStyleSheet("background-color: #e74c3c;");
    deleteBtn->setCursor(Qt::PointingHandCursor);
    listLayout->addWidget(deleteBtn);

    mainLayout->addWidget(listGroup);

    connect(addBtn,    &QPushButton::clicked, this, &MainWindow::onAddEventClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteEventClicked);
    connect(syncBtn,   &QPushButton::clicked, this, &MainWindow::onSyncClicked);
    connect(eventListWidget, &QListWidget::itemSelectionChanged, this, &MainWindow::onSelectionChanged);
    connect(m_networkManager, &NetworkManager::syncSuccess, this, &MainWindow::onSyncSuccess);
    connect(m_networkManager, &NetworkManager::syncError, this, &MainWindow::onSyncError);

    loadEventsFromFile();
    refreshEventList();
}

// ── RE-ADDED THE MISSING LOGIC BELOW ──

void MainWindow::onAddEventClicked() {
    QString title = titleInput->text().trimmed();
    if (title.isEmpty()) {
        QMessageBox::warning(this, "Missing Title", "Please enter an event title.");
        return;
    }
    QDateTime dt = dateTimeEdit->dateTime();
    EventCategory cat = static_cast<EventCategory>(categoryCombo->currentIndex());
    Event newEvent(title, dt, cat);
    m_events.push_back(newEvent);
    sort(m_events.begin(), m_events.end());
    saveEventsToFile();
    refreshEventList();
    titleInput->clear();
}

void MainWindow::onDeleteEventClicked() {
    int row = eventListWidget->currentRow();
    if (row < 0 || row >= static_cast<int>(m_events.size())) return;
    m_events.erase(m_events.begin() + row);
    saveEventsToFile();
    refreshEventList();
    deleteBtn->setEnabled(false);
}

void MainWindow::onSelectionChanged() {
    deleteBtn->setEnabled(eventListWidget->currentRow() >= 0);
}

void MainWindow::onSyncClicked() {
    SyncDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;
    syncBtn->setEnabled(false);
    syncBtn->setText("Syncing…");
    m_networkManager->syncWithGmail(dlg.email(), dlg.appPassword());
}

void MainWindow::onSyncSuccess(const QString& data) {
    syncBtn->setEnabled(true);
    syncBtn->setText("Sync with Gmail");
    if (data.isEmpty()) {
        QMessageBox::information(this, "Sync Complete", "No emails found in inbox.");
        return;
    }
    QStringList lines = data.split("\n", Qt::SkipEmptyParts);
    int found = 0;
    for (const QString& line : lines) {
        Event e = Event::parseEventFromString(line, EventCategory::University);
        if (e.isValid()) {
            m_events.push_back(e);
            found++;
        }
    }
    if (found > 0) {
        sort(m_events.begin(), m_events.end());
        saveEventsToFile();
        refreshEventList();
        QMessageBox::information(this, "Sync Complete", QString("Found %1 event(s).").arg(found));
    }
}

void MainWindow::onSyncError(const QString& error) {
    syncBtn->setEnabled(true);
    syncBtn->setText("Sync with Gmail");
    QMessageBox::critical(this, "Sync Failed", error);
}

void MainWindow::refreshEventList() {
    eventListWidget->clear();
    for (const Event& e : m_events) {
        QString display = QString("[%1]  %2  —  %3")
            .arg(e.categoryString())
            .arg(e.title())
            .arg(e.dateTime().toString("dd/MM/yyyy  hh:mm AP"));
        eventListWidget->addItem(display);
    }
}

void MainWindow::saveEventsToFile() {
    QFile file("events.csv");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    for (const Event& e : m_events) {
        out << e.title() << "|"
            << e.dateTime().toString("dd/MM/yyyy HH:mm") << "|"
            << static_cast<int>(e.category()) << "\n";
    }
    file.close();
}

void MainWindow::loadEventsFromFile() {
    QFile file("events.csv");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QStringList parts = in.readLine().split("|");
        if (parts.size() == 3) {
            QString title = parts[0];
            QDateTime dt  = QDateTime::fromString(parts[1], "dd/MM/yyyy HH:mm");
            bool ok;
            int catIdx = parts[2].toInt(&ok);
            if (ok && dt.isValid()) {
                EventCategory cat = static_cast<EventCategory>(catIdx);
                m_events.emplace_back(title, dt, cat);
            }
        }
    }
    sort(m_events.begin(), m_events.end());
    file.close();
}

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
    setMinimumSize(700, 550);

    m_networkManager = new NetworkManager(this);

    // ── Central widget ────────────────────────────────────────────────────────
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // ── Top bar: Sync button ──────────────────────────────────────────────────
    QHBoxLayout *topBar = new QHBoxLayout();
    QLabel *appTitle = new QLabel("📅  Smart Prioritized Calendar", this);
    QFont titleFont = appTitle->font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    appTitle->setFont(titleFont);

    syncBtn = new QPushButton("Sync with Gmail", this);
    syncBtn->setMinimumHeight(32);

    topBar->addWidget(appTitle);
    topBar->addStretch();
    topBar->addWidget(syncBtn);
    mainLayout->addLayout(topBar);

    // ── Add Event group ───────────────────────────────────────────────────────
    QGroupBox *addGroup = new QGroupBox("Add New Event", this);
    QGridLayout *addLayout = new QGridLayout(addGroup);
    addLayout->setSpacing(8);

    // Row 0: Title label + input
    addLayout->addWidget(new QLabel("Event Title:", this), 0, 0);
    titleInput = new QLineEdit(this);
    titleInput->setPlaceholderText("e.g. Project Meeting");
    titleInput->setMinimumHeight(30);
    addLayout->addWidget(titleInput, 0, 1, 1, 2);

    // Row 1: Date & time (calendar picker)
    addLayout->addWidget(new QLabel("Date & Time:", this), 1, 0);
    dateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    dateTimeEdit->setDisplayFormat("dd/MM/yyyy  hh:mm AP");
    dateTimeEdit->setCalendarPopup(true);   // shows a calendar widget on click
    dateTimeEdit->setMinimumHeight(30);
    dateTimeEdit->setMinimumDate(QDate::currentDate());  // prevent past dates
    addLayout->addWidget(dateTimeEdit, 1, 1);

    // Row 1: Category
    addLayout->addWidget(new QLabel("Category:", this), 1, 2);  // reuse row 1, col 2
    // Adjust: put category in row 2
    addLayout->addWidget(new QLabel("Category:", this), 2, 0);
    categoryCombo = new QComboBox(this);
    categoryCombo->addItems({"University", "Personal", "Other"});
    categoryCombo->setMinimumHeight(30);
    addLayout->addWidget(categoryCombo, 2, 1);

    addBtn = new QPushButton("Add Event", this);
    addBtn->setMinimumHeight(30);
    addLayout->addWidget(addBtn, 2, 2);

    // Remove the duplicate "Category:" label added earlier (row 1, col 2)
    // (Simpler: just restructure the grid cleanly)
    mainLayout->addWidget(addGroup);

    // ── Event list ────────────────────────────────────────────────────────────
    QGroupBox *listGroup = new QGroupBox("Upcoming Events  (sorted by priority, then date)", this);
    QVBoxLayout *listLayout = new QVBoxLayout(listGroup);

    eventListWidget = new QListWidget(this);
    eventListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    listLayout->addWidget(eventListWidget);

    deleteBtn = new QPushButton("Delete Selected Event", this);
    deleteBtn->setEnabled(false);
    deleteBtn->setMinimumHeight(30);
    listLayout->addWidget(deleteBtn);

    mainLayout->addWidget(listGroup);

    // ── Signal connections ────────────────────────────────────────────────────
    connect(addBtn,    &QPushButton::clicked, this, &MainWindow::onAddEventClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteEventClicked);
    connect(syncBtn,   &QPushButton::clicked, this, &MainWindow::onSyncClicked);

    connect(eventListWidget, &QListWidget::itemSelectionChanged,
            this, &MainWindow::onSelectionChanged);

    connect(m_networkManager, &NetworkManager::syncSuccess,
            this, &MainWindow::onSyncSuccess);
    connect(m_networkManager, &NetworkManager::syncError,
            this, &MainWindow::onSyncError);

    // ── Load persisted events ────────────────────────────────────────────────
    loadEventsFromFile();
    refreshEventList();
}

// ── Add event ─────────────────────────────────────────────────────────────────
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

// ── Delete event ──────────────────────────────────────────────────────────────
void MainWindow::onDeleteEventClicked() {
    int row = eventListWidget->currentRow();
    if (row < 0 || row >= static_cast<int>(m_events.size())) return;

    m_events.erase(m_events.begin() + row);
    saveEventsToFile();
    refreshEventList();
    deleteBtn->setEnabled(false);
}

// ── Enable delete button only when something is selected ──────────────────────
void MainWindow::onSelectionChanged() {
    deleteBtn->setEnabled(eventListWidget->currentRow() >= 0);
}

// ── Sync: show credential dialog, then trigger worker ─────────────────────────
void MainWindow::onSyncClicked() {
    SyncDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    syncBtn->setEnabled(false);
    syncBtn->setText("Syncing…");
    m_networkManager->syncWithGmail(dlg.email(), dlg.appPassword());
}

// ── Handle successful IMAP fetch ──────────────────────────────────────────────
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
        QMessageBox::information(this, "Sync Complete",
            QString("Found %1 event(s) in your recent emails.").arg(found));
    } else {
        QMessageBox::information(this, "Sync Complete",
            "Checked recent emails but found no dates in the expected format (DD/MM/YYYY HH:MM).");
    }
}

// ── Handle IMAP error ─────────────────────────────────────────────────────────
void MainWindow::onSyncError(const QString& error) {
    syncBtn->setEnabled(true);
    syncBtn->setText("Sync with Gmail");
    QMessageBox::critical(this, "Sync Failed", error);
}

// ── Refresh list display ──────────────────────────────────────────────────────
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

// ── Persist events to file ────────────────────────────────────────────────────
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

// ── Load events from file ─────────────────────────────────────────────────────
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

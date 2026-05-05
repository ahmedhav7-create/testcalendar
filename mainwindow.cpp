#include "mainwindow.hpp"
#include "syncdialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <QRegularExpression>
#include <QListWidgetItem>
#include <QProgressBar>
#include <QStatusBar>
#include <algorithm>

using namespace std;

// Colour coding per category
static QColor categoryColor(EventCategory cat) {
    switch (cat) {
        case EventCategory::University: return QColor("#1565C0"); // deep blue
        case EventCategory::Personal:   return QColor("#2E7D32"); // deep green
        case EventCategory::Other:      return QColor("#6A1B9A"); // deep purple
    }
    return QColor("#444");
}

static QString categoryBadgeStyle(EventCategory cat) {
    QString bg, fg;
    switch (cat) {
        case EventCategory::University: bg="#BBDEFB"; fg="#0D47A1"; break;
        case EventCategory::Personal:   bg="#C8E6C9"; fg="#1B5E20"; break;
        case EventCategory::Other:      bg="#E1BEE7"; fg="#4A148C"; break;
    }
    return QString("background:%1; color:%2; border-radius:4px;"
                   " padding:2px 6px; font-size:11px; font-weight:bold;").arg(bg, fg);
}

// Global stylesheet
static const QString APP_STYLE = R"(
QMainWindow {
    background: #F5F7FA;
}
QWidget {
    font-family: 'Segoe UI', Arial, sans-serif;
    font-size: 13px;
    color: #212121;
}
QTabWidget::pane {
    border: 1px solid #CFD8DC;
    border-radius: 6px;
    background: #FFFFFF;
}
QTabBar::tab {
    background: #ECEFF1;
    border: 1px solid #CFD8DC;
    border-bottom: none;
    border-top-left-radius: 6px;
    border-top-right-radius: 6px;
    padding: 8px 20px;
    margin-right: 2px;
    color: #546E7A;
    font-weight: bold;
}
QTabBar::tab:selected {
    background: #FFFFFF;
    color: #1565C0;
    border-bottom: 2px solid #1565C0;
}
QTabBar::tab:hover:!selected {
    background: #E3F2FD;
    color: #1565C0;
}
QListWidget {
    border: 1px solid #CFD8DC;
    border-radius: 6px;
    background: #FFFFFF;
    outline: none;
    padding: 4px;
}
QListWidget::item {
    border-radius: 5px;
    padding: 10px 12px;
    margin: 3px 2px;
    border-left: 4px solid transparent;
    background: #FAFAFA;
}
QListWidget::item:selected {
    background: #E3F2FD;
    border-left: 4px solid #1565C0;
    color: #0D47A1;
}
QListWidget::item:hover:!selected {
    background: #F1F8FF;
}
QLineEdit, QDateTimeEdit, QComboBox {
    border: 1px solid #B0BEC5;
    border-radius: 5px;
    padding: 6px 10px;
    background: #FFFFFF;
    selection-background-color: #BBDEFB;
    min-height: 28px;
}
QLineEdit:focus, QDateTimeEdit:focus, QComboBox:focus {
    border: 2px solid #1565C0;
}
QLineEdit:hover, QDateTimeEdit:hover, QComboBox:hover {
    border-color: #78909C;
}
QComboBox::drop-down {
    border: none;
    width: 24px;
}
QComboBox::down-arrow {
    width: 12px;
    height: 12px;
}
QPushButton {
    border-radius: 5px;
    padding: 7px 18px;
    font-weight: bold;
    min-height: 30px;
}
QPushButton#addBtn {
    background: #1565C0;
    color: white;
    border: none;
}
QPushButton#addBtn:hover    { background: #1976D2; }
QPushButton#addBtn:pressed  { background: #0D47A1; }
QPushButton#deleteBtn {
    background: #FFFFFF;
    color: #C62828;
    border: 2px solid #EF9A9A;
}
QPushButton#deleteBtn:hover   { background: #FFEBEE; border-color:#C62828; }
QPushButton#deleteBtn:pressed { background: #FFCDD2; }
QPushButton#deleteBtn:disabled {
    color: #BDBDBD;
    border-color: #E0E0E0;
    background: #FAFAFA;
}
QPushButton#syncBtn {
    background: #FFFFFF;
    color: #1565C0;
    border: 2px solid #90CAF9;
}
QPushButton#syncBtn:hover   { background: #E3F2FD; border-color:#1565C0; }
QPushButton#syncBtn:pressed { background: #BBDEFB; }
QPushButton#syncBtn:disabled { color:#90A4AE; border-color:#CFD8DC; }
QLabel#hintLabel {
    color: #78909C;
    font-size: 11px;
}
QLabel#sectionLabel {
    color: #546E7A;
    font-size: 11px;
    font-weight: bold;
}
QStatusBar {
    background: #ECEFF1;
    color: #546E7A;
    font-size: 11px;
    border-top: 1px solid #CFD8DC;
}
QProgressBar {
    border: none;
    border-radius: 3px;
    background: #CFD8DC;
    max-height: 4px;
    text-align: center;
}
QProgressBar::chunk {
    background: #1565C0;
    border-radius: 3px;
}
)";


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("Smart Prioritized Calendar");
    setMinimumSize(720, 560);
    setStyleSheet(APP_STYLE);

    m_networkManager = new NetworkManager(this);

    // Status bar (shows feedback messages at the bottom)
    statusBar()->showMessage("Ready");

    // Central widget
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *outerLayout = new QVBoxLayout(central);
    outerLayout->setContentsMargins(16, 16, 16, 12);
    outerLayout->setSpacing(10);

    // Header bar
    QWidget *header = new QWidget(this);
    header->setStyleSheet("background:#1565C0; border-radius:8px;");
    QHBoxLayout *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(16, 10, 16, 10);

    QLabel *appTitle = new QLabel("📅   Smart Prioritized Calendar", header);
    QFont tf = appTitle->font();
    tf.setPointSize(14);
    tf.setBold(true);
    appTitle->setFont(tf);
    appTitle->setStyleSheet("color:white; background:transparent;");

    syncBtn = new QPushButton("⟳  Sync Gmail", header);
    syncBtn->setObjectName("syncBtn");
    syncBtn->setStyleSheet(
        "QPushButton { background:white; color:#1565C0; border:none;"
        "              border-radius:5px; padding:6px 14px; font-weight:bold; }"
        "QPushButton:hover   { background:#E3F2FD; }"
        "QPushButton:pressed { background:#BBDEFB; }"
        "QPushButton:disabled{ background:#CFD8DC; color:#90A4AE; }"
    );

    headerLayout->addWidget(appTitle);
    headerLayout->addStretch();
    headerLayout->addWidget(syncBtn);
    outerLayout->addWidget(header);

    // Sync progress bar (hidden until sync starts)
    syncProgress = new QProgressBar(this);
    syncProgress->setRange(0, 0);      // indeterminate / busy indicator
    syncProgress->setVisible(false);
    syncProgress->setMaximumHeight(4);
    outerLayout->addWidget(syncProgress);

    // QTabWidget
    tabWidget = new QTabWidget(this);

    // Tab 1: Upcoming Events
    QWidget *eventsTab = new QWidget();
    QVBoxLayout *eventsLayout = new QVBoxLayout(eventsTab);
    eventsLayout->setContentsMargins(12, 12, 12, 12);
    eventsLayout->setSpacing(8);

    // Legend row
    QHBoxLayout *legendRow = new QHBoxLayout();
    legendRow->setSpacing(12);
    auto makeLegend = [](const QString& label, const QString& style) {
        QLabel *l = new QLabel(label);
        l->setStyleSheet(style + " border-radius:4px; padding:2px 8px; font-size:11px; font-weight:bold;");
        l->setFixedHeight(22);
        return l;
    };
    legendRow->addWidget(new QLabel("Priority:"));
    legendRow->addWidget(makeLegend("University", "background:#BBDEFB; color:#0D47A1;"));
    legendRow->addWidget(makeLegend("Personal",   "background:#C8E6C9; color:#1B5E20;"));
    legendRow->addWidget(makeLegend("Other",      "background:#E1BEE7; color:#4A148C;"));
    legendRow->addStretch();

    QLabel *countLabel = new QLabel("0 events", eventsTab);
    countLabel->setObjectName("sectionLabel");
    eventCountLabel = countLabel;
    legendRow->addWidget(eventCountLabel);

    eventsLayout->addLayout(legendRow);

    // Event list
    eventListWidget = new QListWidget(eventsTab);
    eventListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    eventListWidget->setSpacing(2);
    eventsLayout->addWidget(eventListWidget);

    // Delete button (right-aligned)
    QHBoxLayout *deleteBtnRow = new QHBoxLayout();
    deleteBtnRow->addStretch();
    deleteBtn = new QPushButton("🗑  Delete Selected", eventsTab);
    deleteBtn->setObjectName("deleteBtn");
    deleteBtn->setEnabled(false);
    deleteBtn->setMaximumWidth(200);
    deleteBtnRow->addWidget(deleteBtn);
    eventsLayout->addLayout(deleteBtnRow);

    tabWidget->addTab(eventsTab, "📋  Upcoming Events");

    //Tab 2: Add Event
    QWidget *addTab = new QWidget();
    QVBoxLayout *addOuter = new QVBoxLayout(addTab);
    addOuter->setContentsMargins(20, 20, 20, 20);
    addOuter->setSpacing(14);

    // Card-style container
    QWidget *card = new QWidget(addTab);
    card->setStyleSheet("background:#FFFFFF; border-radius:8px;"
                        "border:1px solid #CFD8DC;");
    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 20, 20, 20);
    cardLayout->setSpacing(14);

    QLabel *formTitle = new QLabel("New Event Details", card);
    QFont ft = formTitle->font();
    ft.setPointSize(12);
    ft.setBold(true);
    formTitle->setFont(ft);
    formTitle->setStyleSheet("color:#1565C0; border:none;");
    cardLayout->addWidget(formTitle);

    QFrame *divider = new QFrame(card);
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color:#E0E0E0;");
    cardLayout->addWidget(divider);

    // Title field
    QLabel *titleLbl = new QLabel("Event Title *", card);
    titleLbl->setObjectName("sectionLabel");
    cardLayout->addWidget(titleLbl);
    titleInput = new QLineEdit(card);
    titleInput->setPlaceholderText("e.g.  Project presentation, Doctor appointment…");
    cardLayout->addWidget(titleInput);

    // Date & time field
    QLabel *dateLbl = new QLabel("Date & Time *", card);
    dateLbl->setObjectName("sectionLabel");
    cardLayout->addWidget(dateLbl);
    dateTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), card);
    dateTimeEdit->setDisplayFormat("dd / MM / yyyy    hh:mm AP");
    dateTimeEdit->setCalendarPopup(true);
    dateTimeEdit->setMinimumDate(QDate::currentDate());
    dateTimeEdit->setMinimumWidth(260);
    cardLayout->addWidget(dateTimeEdit);

    QLabel *dateHint = new QLabel("Click the clock icon to open a calendar picker.", card);
    dateHint->setObjectName("hintLabel");
    cardLayout->addWidget(dateHint);

    // Category field
    QLabel *catLbl = new QLabel("Category *", card);
    catLbl->setObjectName("sectionLabel");
    cardLayout->addWidget(catLbl);
    categoryCombo = new QComboBox(card);
    categoryCombo->addItem("🎓  University   — highest priority");
    categoryCombo->addItem("🏠  Personal     — medium priority");
    categoryCombo->addItem("📌  Other        — lowest priority");
    cardLayout->addWidget(categoryCombo);

    // Add button
    cardLayout->addSpacing(6);
    addBtn = new QPushButton("＋  Add Event", card);
    addBtn->setObjectName("addBtn");
    addBtn->setMinimumHeight(38);
    cardLayout->addWidget(addBtn);

    addOuter->addWidget(card);
    addOuter->addStretch();

    tabWidget->addTab(addTab, "➕  Add Event");

    outerLayout->addWidget(tabWidget);

    // Connections
    connect(addBtn,    &QPushButton::clicked, this, &MainWindow::onAddEventClicked);
    connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteEventClicked);
    connect(syncBtn,   &QPushButton::clicked, this, &MainWindow::onSyncClicked);

    connect(eventListWidget, &QListWidget::itemSelectionChanged,
            this, &MainWindow::onSelectionChanged);

    connect(m_networkManager, &NetworkManager::syncSuccess,
            this, &MainWindow::onSyncSuccess);
    connect(m_networkManager, &NetworkManager::syncError,
            this, &MainWindow::onSyncError);

    loadEventsFromFile();
    refreshEventList();
}


void MainWindow::onAddEventClicked() {
    QString title = titleInput->text().trimmed();
    if (title.isEmpty()) {
        // Highlight the field instead of a full dialog
        titleInput->setStyleSheet("border:2px solid #C62828; border-radius:5px; padding:6px 10px;");
        statusBar()->showMessage("⚠  Please enter an event title.", 4000);
        return;
    }
    // Reset field style
    titleInput->setStyleSheet("");

    QDateTime dt  = dateTimeEdit->dateTime();
    EventCategory cat = static_cast<EventCategory>(categoryCombo->currentIndex());

    m_events.emplace_back(title, dt, cat);
    sort(m_events.begin(), m_events.end());
    saveEventsToFile();
    refreshEventList();
    titleInput->clear();

    tabWidget->setCurrentIndex(0);
    statusBar()->showMessage(QString("✔  '%1' added successfully.").arg(title), 4000);
}

void MainWindow::onDeleteEventClicked() {
    int row = eventListWidget->currentRow();
    if (row < 0 || row >= static_cast<int>(m_events.size())) return;

    QString removedTitle = m_events[row].title();
    m_events.erase(m_events.begin() + row);
    saveEventsToFile();
    refreshEventList();
    deleteBtn->setEnabled(false);
    statusBar()->showMessage(QString("🗑  '%1' deleted.").arg(removedTitle), 4000);
}

void MainWindow::onSelectionChanged() {
    deleteBtn->setEnabled(eventListWidget->currentRow() >= 0);
}

void MainWindow::onSyncClicked() {
    SyncDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    syncBtn->setEnabled(false);
    syncBtn->setText("Syncing…");
    syncProgress->setVisible(true);
    statusBar()->showMessage("⟳  Connecting to Gmail…");
    m_networkManager->syncWithGmail(dlg.email(), dlg.appPassword());
}

void MainWindow::onSyncSuccess(const QString& data) {
    syncBtn->setEnabled(true);
    syncBtn->setText("⟳  Sync Gmail");
    syncProgress->setVisible(false);

    if (data.isEmpty()) {
        statusBar()->showMessage("Sync complete — no emails in inbox.", 5000);
        return;
    }

    QStringList lines = data.split("\n", Qt::SkipEmptyParts);
    int found = 0;

    static QRegularExpression base64Line(R"(^[A-Za-z0-9+/]{20,}={0,2}$)");
    static QRegularExpression mimeBoundary(R"(^--.*)");
    static QRegularExpression headerLine(R"(^[A-Za-z\-]+:.*)");

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        if (base64Line.match(trimmed).hasMatch())  continue;
        if (mimeBoundary.match(trimmed).hasMatch()) continue;
        if (headerLine.match(trimmed).hasMatch())  continue;

        Event e = Event::parseEventFromString(trimmed, EventCategory::University);
        if (e.isValid()) {
            bool dup = false;
            for (const Event& ex : m_events)
                if (ex.title()==e.title() && ex.dateTime()==e.dateTime()) { dup=true; break; }
            if (!dup) { m_events.push_back(e); found++; }
        }
    }

    if (found > 0) {
        sort(m_events.begin(), m_events.end());
        saveEventsToFile();
        refreshEventList();
        statusBar()->showMessage(
            QString("✔  Sync complete — %1 new event(s) imported from Gmail.").arg(found), 6000);
        QMessageBox::information(this, "Sync Complete",
            QString("Found %1 new event(s) in your recent emails.\n\n"
                    "They have been added to your calendar.").arg(found));
    } else {
        statusBar()->showMessage(
            "Sync complete — no dates found in recent emails.", 5000);
        QMessageBox::information(this, "Sync Complete",
            "No new events found in your recent emails.\n\n"
            "Make sure the email body contains a date in the format:\n"
            "DD/MM/YYYY HH:MM   (e.g. 25/12/2026 09:00)");
    }
}

void MainWindow::onSyncError(const QString& error) {
    syncBtn->setEnabled(true);
    syncBtn->setText("⟳  Sync Gmail");
    syncProgress->setVisible(false);
    statusBar()->showMessage("✖  Sync failed: " + error, 6000);
    QMessageBox::critical(this, "Sync Failed", error);
}

// Refresh list — each item is colour-coded by category
void MainWindow::refreshEventList() {
    eventListWidget->clear();

    for (const Event& e : m_events) {
        // Two-line display: title on top, date + category badge below
        QString line1 = e.title();
        QString line2 = e.dateTime().toString("ddd, dd MMM yyyy  •  hh:mm AP")
                      + "   [" + e.categoryString() + "]";

        QListWidgetItem *item = new QListWidgetItem(line1 + "\n" + line2);

        // Left-border colour via foreground (used as accent indicator)
        item->setForeground(categoryColor(e.category()));

        // Tooltip so the user can hover for details
        item->setToolTip(QString("Title: %1\nDate:  %2\nCategory: %3")
            .arg(e.title())
            .arg(e.dateTime().toString("dd/MM/yyyy hh:mm AP"))
            .arg(e.categoryString()));

        item->setSizeHint(QSize(0, 58));   // fixed row height for two-line items
        eventListWidget->addItem(item);
    }

    // Update event count label
    int n = static_cast<int>(m_events.size());
    eventCountLabel->setText(QString("%1 event%2").arg(n).arg(n==1?"":"s"));
}

void MainWindow::saveEventsToFile() {
    QFile file("events.csv");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    for (const Event& e : m_events)
        out << e.title() << "|"
            << e.dateTime().toString("dd/MM/yyyy HH:mm") << "|"
            << static_cast<int>(e.category()) << "\n";
    file.close();
}

void MainWindow::loadEventsFromFile() {
    QFile file("events.csv");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QStringList parts = in.readLine().split("|");
        if (parts.size() == 3) {
            QDateTime dt = QDateTime::fromString(parts[1], "dd/MM/yyyy HH:mm");
            bool ok; int idx = parts[2].toInt(&ok);
            if (ok && dt.isValid())
                m_events.emplace_back(parts[0], dt, static_cast<EventCategory>(idx));
        }
    }
    sort(m_events.begin(), m_events.end());
    file.close();
}

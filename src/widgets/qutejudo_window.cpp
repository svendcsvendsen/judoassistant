#include <QMenu>
#include <QMenuBar>
#include <QTabWidget>
#include <QAction>
#include <QDesktopServices>
#include <QFileDialog>
#include <QtDebug>
#include <QStandardPaths>
#include <QMessageBox>

#include <fstream>

#include "widgets/qutejudo_window.hpp"
#include "widgets/tournament_widget.hpp"
#include "widgets/players_widget.hpp"
#include "widgets/categories_widget.hpp"
#include "widgets/tatamis_widget.hpp"
#include "widgets/matches_widget.hpp"
#include "widgets/import_players_csv_dialog.hpp"
#include "widgets/import_helpers/csv_reader.hpp"
#include "config/web.hpp"
#include "exception.hpp"

QutejudoWindow::QutejudoWindow() {
    // TODO: Add todostack sidebar
    createStatusBar();
    createTournamentMenu();
    createEditMenu();
    createViewMenu();
    createPreferencesMenu();
    createHelpMenu();

    QTabWidget * tabWidget = new QTabWidget(this);
    tabWidget->addTab(new TournamentWidget(mStoreHandler), tr("Tournament"));
    tabWidget->addTab(new PlayersWidget(mStoreHandler), tr("Players"));
    tabWidget->addTab(new CategoriesWidget(mStoreHandler), tr("Categories"));
    tabWidget->addTab(new TatamisWidget(mStoreHandler), tr("Tatamis"));
    tabWidget->addTab(new MatchesWidget(mStoreHandler), tr("Matches"));
    tabWidget->setCurrentIndex(1);
    tabWidget->setTabPosition(QTabWidget::TabPosition::West);

    setCentralWidget(tabWidget);

    resize(250,250);
    setWindowTitle(tr("Qutejudo"));
}

void QutejudoWindow::createStatusBar() {
    statusBar();
}

void QutejudoWindow::createTournamentMenu() {
    QMenu *menu = menuBar()->addMenu(tr("Tournament"));

    {
        QAction *action = new QAction(tr("New"), this);
        action->setShortcuts(QKeySequence::New);
        action->setStatusTip(tr("Create a new tournament"));
        connect(action, &QAction::triggered, this, &QutejudoWindow::newTournament);
        menu->addAction(action);
    }

    menu->addSeparator();

    {
        QAction *action = new QAction(tr("Open.."), this);
        action->setShortcuts(QKeySequence::Open);
        action->setStatusTip(tr("Load tournament from a file"));
        connect(action, &QAction::triggered, this, &QutejudoWindow::openTournament);
        menu->addAction(action);
    }

    {
        // TODO: Implement open recent
        // QMenu * submenu = menu->addMenu("Open Recent");
    }

    {
        QMenu *submenu = menu->addMenu(tr("Import.."));

        {
            QAction *action = new QAction(tr("Import players.."), this);
            action->setStatusTip(tr("Import players from a file"));
            connect(action, &QAction::triggered, this, &QutejudoWindow::openImportPlayers);
            submenu->addAction(action);
        }
    }

    menu->addSeparator();

    {
        QAction *action = new QAction(tr("Save"), this);
        action->setShortcuts(QKeySequence::Save);
        action->setStatusTip(tr("Save tournament to a file"));
        connect(action, &QAction::triggered, this, &QutejudoWindow::saveTournament);
        menu->addAction(action);
    }

    {
        QAction *action = new QAction(tr("Save As.."), this);
        action->setShortcuts(QKeySequence::SaveAs);
        action->setStatusTip(tr("Save the tournament to file"));
        connect(action, &QAction::triggered, this, &QutejudoWindow::saveAsTournament);
        menu->addAction(action);
    }

    menu->addSeparator();

    {
        QAction *action = new QAction(tr("Quit"), this);
        action->setShortcuts(QKeySequence::Quit);
        action->setStatusTip(tr("Quit Qutejudo"));
        connect(action, &QAction::triggered, this, &QutejudoWindow::quit);
        menu->addAction(action);
    }
}

void QutejudoWindow::createEditMenu() {
    QMenu *menu = menuBar()->addMenu(tr("Edit"));

    {
        QAction *action = new QAction(tr("Undo"), this);
        action->setShortcuts(QKeySequence::Undo);
        action->setStatusTip(tr("Undo last action"));
        action->setEnabled(mStoreHandler.canUndo());
        menu->addAction(action);
        connect(&mStoreHandler, &MasterStoreHandler::undoStatusChanged, action, &QAction::setEnabled);
        connect(action, &QAction::triggered, &mStoreHandler, &MasterStoreHandler::undo);
    }

    {
        QAction *action = new QAction(tr("Redo"), this);
        action->setShortcuts(QKeySequence::Redo);
        action->setStatusTip(tr("Redo the last undone action"));
        action->setEnabled(mStoreHandler.canRedo());
        menu->addAction(action);
        connect(&mStoreHandler, &MasterStoreHandler::redoStatusChanged, action, &QAction::setEnabled);
        connect(action, &QAction::triggered, &mStoreHandler, &MasterStoreHandler::redo);
    }
}

void QutejudoWindow::createViewMenu() {
    QMenu *menu = menuBar()->addMenu(tr("View"));
}

void QutejudoWindow::createPreferencesMenu() {
    QMenu *menu = menuBar()->addMenu(tr("Preferences"));
    {
        QMenu *submenu = menu->addMenu("Language");
        QAction *englishAction = new QAction(tr("English"), this);
        submenu->addAction(englishAction);
    }
}

void QutejudoWindow::createHelpMenu() {
    QMenu *menu = menuBar()->addMenu(tr("Help"));
    {
        QAction *action = new QAction(tr("Qutejudo Home Page"), this);
        connect(action, &QAction::triggered, this, &QutejudoWindow::openHomePage);
        menu->addAction(action);
    }
    {
        QAction *action = new QAction(tr("Qutejudo Manual"), this);
        connect(action, &QAction::triggered, this, &QutejudoWindow::openManual);
        menu->addAction(action);
    }

    menu->addSeparator();

    {
        QAction *action = new QAction(tr("Report an Issue"), this);
        connect(action, &QAction::triggered, this, &QutejudoWindow::openReportIssue);

        menu->addAction(action);
    }
    {
        QAction *action = new QAction(tr("About"), this);
        connect(action, &QAction::triggered, this, &QutejudoWindow::showAboutDialog);
        menu->addAction(action);
    }
}

void QutejudoWindow::openHomePage() {
    if(QDesktopServices::openUrl(Config::HOME_PAGE_URL))
        return;

    // QMessageBox::warning(this, tr("Unable to open home page."));
}

void QutejudoWindow::openManual() {
    if(QDesktopServices::openUrl(Config::MANUAL_URL))
        return;

    QMessageBox::warning(this, tr("Unable to open manual."), tr("The manual is available at"));
}

void QutejudoWindow::openReportIssue() {
    if(QDesktopServices::openUrl(Config::REPORT_ISSUE_URL))
        return;

    // QMessageBox::warning(this, tr("Unable to open report issue page."));
}

void QutejudoWindow::writeTournament() {
    if (!mStoreHandler.write(mFileName))
        QMessageBox::warning(this, tr("Unable to open file"), tr("The selected file could not be opened."));
    else
        statusBar()->showMessage(tr("Saved tournament to file"));
}

void QutejudoWindow::readTournament() {
    readTournament(mFileName);
}

void QutejudoWindow::readTournament(const QString &fileName) {
    mFileName = fileName;
    if (!mStoreHandler.read(fileName))
        QMessageBox::warning(this, tr("Unable to open file"), tr("The selected file could not be opened."));
    else
        statusBar()->showMessage(tr("Opened tournament from file"));
}

void QutejudoWindow::newTournament() {
    if (mStoreHandler.isDirty()) {
        auto reply = QMessageBox::question(this, tr("Unsaved changes"), tr("The current tournament has unsaved changes. Would you still like to continue?"), QMessageBox::Yes | QMessageBox::Cancel);
        if (reply == QMessageBox::Cancel)
            return;
    }

    mFileName = "";
    mStoreHandler.reset();
}

void QutejudoWindow::openTournament() {
    if (mStoreHandler.isDirty()) {
        auto reply = QMessageBox::question(this, tr("Unsaved changes"), tr("The current tournament has unsaved changes. Would you still like to continue?"), QMessageBox::Yes | QMessageBox::Cancel);
        if (reply == QMessageBox::Cancel)
            return;
    }
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Tournament"), QStandardPaths::writableLocation(QStandardPaths::HomeLocation), tr("Tournament Files (*.qj);;All Files (*)"));

    if (fileName.isEmpty())
        return;

    readTournament(fileName);
}

void QutejudoWindow::quit() {
    if (mStoreHandler.isDirty()) {
        auto reply = QMessageBox::question(this, tr("Unsaved changes"), tr("The current tournament has unsaved changes. Would you still like to exit without saving?"), QMessageBox::Yes | QMessageBox::Cancel);
        if (reply == QMessageBox::Cancel)
            return;
    }

    QCoreApplication::exit();
}

void QutejudoWindow::saveTournament() {
    if (mFileName.isEmpty())
        saveAsTournament();
    else
        writeTournament();
}

void QutejudoWindow::saveAsTournament() {
    // TODO: Append .qj if not already
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Tournament"), QStandardPaths::writableLocation(QStandardPaths::HomeLocation), tr("Tournament Files (*.qj);;All Files (*)"));

    if (fileName.isEmpty())
        return;

    mFileName = fileName;
    writeTournament();
}

void QutejudoWindow::showAboutDialog() {
    QMessageBox::about(this, tr("Qutejudo - About"), tr("TODO"));
}

void QutejudoWindow::openImportPlayers() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Import players"), QStandardPaths::writableLocation(QStandardPaths::HomeLocation), tr("Comma-separated (CSV) Files (*.csv);;All Files (*)"));

    if (fileName.isEmpty())
        return;

    CSVReader reader(fileName);
    if (!reader.isOpen()) {
        QMessageBox::warning(this, tr("Unable to open file"), tr("The given file could not be opened."));
        return;
    }


    ImportPlayersCSVDialog dialog(mStoreHandler, &reader);

    dialog.exec();
}


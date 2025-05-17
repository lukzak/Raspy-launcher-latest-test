#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>

class AddonRepoManager : public QDialog {
    Q_OBJECT

public:
    AddonRepoManager(QWidget* parent = nullptr);

private slots:
    void onAddRepoClicked();
    void onLoadRepoClicked();
    void onDeleteRepoClicked();
    void onSelectAllClicked();
    void onUnselectAllClicked();
    void onInstallAddonsClicked();

private:
    QString currentRepoContent;

    void loadRepoList();
    void saveRepoToFile(const QString& name, const QString& url);
    bool downloadFile(const QString& url, QString& content);
    void showError(const QString& message);
    void showInfo(const QString& message);
    void parseAndDisplayAddons(const QString& fileContent);
    QString getRepoFilePath() const;

    QLineEdit* urlEdit;
    QPushButton* addRepoButton;

    QComboBox* repoDropdown;
    QPushButton* loadRepoButton;
    QPushButton* deleteRepoButton;

    QListWidget* addonList;
    QPushButton* selectAllButton;
    QPushButton* unselectAllButton;
    QPushButton* installAddonsButton;

    struct AddonEntry {
        QString displayName;
        QString url;
    };

    QVector<AddonEntry> currentAddons;
};

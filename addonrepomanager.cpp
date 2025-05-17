#include "addonrepomanager.h"
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <unzip.h>
#include <QProgressDialog>
#include <QApplication>


bool unzipWithMiniZip(const QString& zipPath, const QString& extractTo) {
    unzFile zipFile = unzOpen(zipPath.toUtf8().constData());
    if (!zipFile) {
        qWarning() << "Cannot open zip file:" << zipPath;
        return false;
    }

    QDir().mkpath(extractTo);

    if (unzGoToFirstFile(zipFile) != UNZ_OK) {
        qWarning() << "Cannot go to first file in zip:" << zipPath;
        unzClose(zipFile);
        return false;
    }

    do {
        char filename[512];
        unz_file_info fileInfo;

        if (unzGetCurrentFileInfo(zipFile, &fileInfo, filename, sizeof(filename), nullptr, 0, nullptr, 0) != UNZ_OK) {
            qWarning() << "Failed to get file info.";
            break;
        }

        QString relativePath = QString::fromUtf8(filename);

        if (relativePath.endsWith('/')) {
            // Skip directories, we will create them implicitly
            continue;
        }

        QString fullOutputPath = extractTo + "/" + relativePath;
        QFileInfo outFileInfo(fullOutputPath);
        QDir().mkpath(outFileInfo.path());  // Create intermediate folders

        if (unzOpenCurrentFile(zipFile) != UNZ_OK) {
            qWarning() << "Cannot open file inside zip:" << filename;
            break;
        }

        QFile outFile(fullOutputPath);
        if (!outFile.open(QIODevice::WriteOnly)) {
            qWarning() << "Cannot write extracted file:" << fullOutputPath;
            unzCloseCurrentFile(zipFile);
            break;
        }

        const int bufferSize = 8192;
        char buffer[bufferSize];
        int bytesRead = 0;

        do {
            bytesRead = unzReadCurrentFile(zipFile, buffer, bufferSize);
            if (bytesRead < 0) {
                qWarning() << "Read error for file:" << filename;
                unzCloseCurrentFile(zipFile);
                outFile.close();
                break;
            }
            outFile.write(buffer, bytesRead);
        } while (bytesRead > 0);

        outFile.close();
        unzCloseCurrentFile(zipFile);
    } while (unzGoToNextFile(zipFile) == UNZ_OK);

    unzClose(zipFile);
    return true;
}

bool initializingRepoList = false;


AddonRepoManager::AddonRepoManager(QWidget* parent)
    : QDialog(parent) {
    setWindowTitle("Manage Addon Repos");
    resize(600, 400);

    urlEdit = new QLineEdit(this);
    urlEdit->setPlaceholderText("Enter repo URL...");
    urlEdit->setMinimumWidth(500);

    addRepoButton = new QPushButton("Add Repo", this);

    QHBoxLayout* urlLayout = new QHBoxLayout();
    urlLayout->addWidget(urlEdit);
    urlLayout->addWidget(addRepoButton);

    repoDropdown = new QComboBox(this);
    deleteRepoButton = new QPushButton("Delete Repo", this);

    QHBoxLayout* repoLayout = new QHBoxLayout();
    repoLayout->addWidget(repoDropdown);
    repoLayout->addWidget(deleteRepoButton);

    addonList = new QListWidget(this);
    addonList->setSelectionMode(QAbstractItemView::NoSelection);

    selectAllButton = new QPushButton("Select All", this);
    unselectAllButton = new QPushButton("Unselect All", this);
    installAddonsButton = new QPushButton("Install Addons", this);

    QHBoxLayout* addonControlLayout = new QHBoxLayout();
    addonControlLayout->addWidget(selectAllButton);
    addonControlLayout->addWidget(unselectAllButton);
    addonControlLayout->addWidget(installAddonsButton);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(urlLayout);
    mainLayout->addLayout(repoLayout);
    mainLayout->addWidget(addonList);
    mainLayout->addLayout(addonControlLayout);
    setLayout(mainLayout);

    connect(addRepoButton, &QPushButton::clicked, this, &AddonRepoManager::onAddRepoClicked);
    connect(repoDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddonRepoManager::onLoadRepoClicked);
    connect(deleteRepoButton, &QPushButton::clicked, this, &AddonRepoManager::onDeleteRepoClicked);
    connect(selectAllButton, &QPushButton::clicked, this, &AddonRepoManager::onSelectAllClicked);
    connect(unselectAllButton, &QPushButton::clicked, this, &AddonRepoManager::onUnselectAllClicked);
    connect(installAddonsButton, &QPushButton::clicked, this, &AddonRepoManager::onInstallAddonsClicked);

    loadRepoList();
}

void AddonRepoManager::onAddRepoClicked() {
    QString url = urlEdit->text().trimmed();
    if (url.isEmpty()) {
        showError("Please enter a URL.");
        return;
    }

    QString fileContent;
    if (!downloadFile(url, fileContent)) {
        showError("Failed to download the repo file.");
        return;
    }

    // Store the downloaded content in currentRepoContent
    currentRepoContent = fileContent;

    QStringList lines = fileContent.split("\n", Qt::SkipEmptyParts);
    if (lines.isEmpty() || lines[0].trimmed() != "!addon_repo") {
        showError("This is not a valid addon repo.");
        return;
    }

    if (lines.size() < 2) {
        showError("Repo name line is missing.");
        return;
    }

    QString secondLine = lines[1].trimmed();
    QRegularExpression re("<(.*?)>");
    QRegularExpressionMatch match = re.match(secondLine);
    if (!match.hasMatch()) {
        showError("Repo name not found in second line.");
        return;
    }

    QString repoName = match.captured(1);
    saveRepoToFile(repoName, url);
    showInfo(QString("Repo '%1' has been added!").arg(repoName));
    loadRepoList();
}

void AddonRepoManager::onLoadRepoClicked() {
    if (initializingRepoList) return;

    int index = repoDropdown->currentIndex();
    if (index < 0) {
        //showError("No repository selected.");
        return;
    }

    QString repoUrl = repoDropdown->currentData().toString();
    QString repoName = repoDropdown->currentText();

    // Download the repo file (addon_sync.txt equivalent) from the repo URL
    QString repoContent;
    if (!downloadFile(repoUrl, repoContent)) {
        showError("Failed to download repo content from URL: " + repoUrl);
        return;
    }

    // Store the downloaded repo content
    currentRepoContent = repoContent;

    parseAndDisplayAddons(repoContent);
}

void AddonRepoManager::parseAndDisplayAddons(const QString& fileContent) {
    addonList->clear();  // Clear any previous items in the list

    QStringList lines = fileContent.split("\n", Qt::SkipEmptyParts);
    bool insideAddonsSection = false;

    for (const QString& line : lines) {
        if (line.contains("<addons>")) {
            insideAddonsSection = true;
            continue;
        }

        if (line.contains("</addons>")) {
            insideAddonsSection = false;
            break;
        }

        if (insideAddonsSection && line.contains("::")) {
            QStringList parts = line.split("::");
            if (parts.size() == 2) {
                QString displayName = parts[0].trimmed();
                QString addonUrl = parts[1].trimmed();

                // Add to the list with checkbox
                QListWidgetItem* item = new QListWidgetItem(displayName, addonList);
                item->setCheckState(Qt::Unchecked);
                item->setData(Qt::UserRole, addonUrl);
            }
        }
    }
}

void AddonRepoManager::onDeleteRepoClicked() {
    int index = repoDropdown->currentIndex();
    if (index < 0) {
        showError("No repository selected.");
        return;
    }

    QString repoName = repoDropdown->currentText();
    QString repoUrl = repoDropdown->currentData().toString();

    // Confirm deletion
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,
                                  "Confirm Deletion",
                                  "Are you sure you wish to delete \"" + repoName + "\"?",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return; // Cancel deletion
    }

    // Remove from file
    QFile file(getRepoFilePath());
    if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        QTextStream stream(&file);
        QString content = stream.readAll();
        file.resize(0);  // Clear file content

        QStringList lines = content.split("\n", Qt::SkipEmptyParts);
        QStringList newLines;
        bool foundRepo = false;

        for (const QString& line : lines) {
            if (line.contains("::")) {
                QStringList parts = line.split("::");
                if (parts.size() == 2 && parts[1].trimmed() == repoUrl) {
                    foundRepo = true;
                    continue;
                }
            }
            newLines.append(line);
        }

        if (!foundRepo) {
            showError("Repo not found in the list.");
            return;
        }

        for (const QString& line : newLines) {
            stream << line << "\n";
        }

        file.close();
    }

    // Temporarily block signals to prevent index change triggering onLoadRepoClicked
    repoDropdown->blockSignals(true);
    repoDropdown->removeItem(index);
    repoDropdown->blockSignals(false);

    addonList->clear();  // Clear the addon list
    currentRepoContent.clear();  // Reset repo content

    // Manually trigger loading the new selection (if one exists)
    if (repoDropdown->count() > 0) {
        onLoadRepoClicked();
    }
}

void AddonRepoManager::onSelectAllClicked() {
    for (int i = 0; i < addonList->count(); ++i) {
        QListWidgetItem* item = addonList->item(i);
        item->setCheckState(Qt::Checked);
    }
}

void AddonRepoManager::onUnselectAllClicked() {
    for (int i = 0; i < addonList->count(); ++i) {
        QListWidgetItem* item = addonList->item(i);
        item->setCheckState(Qt::Unchecked);
    }
}

void AddonRepoManager::onInstallAddonsClicked() {
    qDebug() << "Install Addons clicked";
    if (currentRepoContent.isEmpty()) {
        showError("No repo loaded.");
        return;
    }

    QString addonsDir = QDir::currentPath() + "/Interface/AddOns";
    QDir().mkpath(addonsDir);

    QStringList lines = currentRepoContent.split('\n', Qt::SkipEmptyParts);
    QMap<QString, QString> addonMap;
    bool insideAddons = false;

    // Parse addon data
    for (const QString& line : lines) {
        if (line.trimmed() == "<addons>") {
            insideAddons = true;
            continue;
        }
        if (line.trimmed() == "</addons>") {
            break;
        }

        if (insideAddons) {
            QRegularExpression re(R"(\[(.*?)\]::(.*))");
            QRegularExpressionMatch match = re.match(line.trimmed());
            if (match.hasMatch()) {
                addonMap.insert(match.captured(1), match.captured(2));
            }
        }
    }

    QStringList failedAddons;
    QList<QListWidgetItem*> selectedItems;

    for (int i = 0; i < addonList->count(); ++i) {
        QListWidgetItem* item = addonList->item(i);
        if (item->checkState() == Qt::Checked) {
            selectedItems.append(item);
        }
    }

    if (selectedItems.isEmpty()) {
        showError("No addons selected.");
        return;
    }

    QProgressDialog progress("Installing addons...", "Cancel", 0, selectedItems.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);

    for (int i = 0; i < selectedItems.size(); ++i) {
        if (progress.wasCanceled()) {
            showInfo("Addon installation canceled.");
            return;
        }

        QListWidgetItem* item = selectedItems[i];
        QString displayName = item->text().remove(QRegularExpression("[\\[\\]]")).trimmed();
        QString addonUrl = addonMap.value(displayName);

        progress.setLabelText(QString("Installing %1...").arg(displayName));
        progress.setValue(i);
        QApplication::processEvents();

        if (addonUrl.isEmpty()) {
            failedAddons << displayName;
            continue;
        }

        // Download
        QNetworkAccessManager manager;
        QNetworkRequest request{ QUrl(addonUrl) };
        QNetworkReply* reply = manager.get(request);
        QEventLoop loop;
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "Failed to download:" << displayName << reply->errorString();
            failedAddons << displayName;
            reply->deleteLater();
            continue;
        }

        QByteArray downloadedData = reply->readAll();
        reply->deleteLater();

        QString savePath = addonsDir + "/" + displayName + ".zip";
        QFile file(savePath);
        if (!file.open(QIODevice::WriteOnly)) {
            qDebug() << "Failed to save:" << savePath;
            failedAddons << displayName;
            continue;
        }

        file.write(downloadedData);
        file.close();

        if (unzipWithMiniZip(savePath, addonsDir)) {
            QFile::remove(savePath);
        } else {
            qDebug() << "Failed to unzip:" << savePath;
            failedAddons << displayName;
        }
    }

    progress.setValue(selectedItems.size());

    if (failedAddons.isEmpty()) {
        showInfo("Addons installed successfully.");
    } else {
        showError("Failed to install: " + failedAddons.join(", "));
    }
}

void AddonRepoManager::loadRepoList() {
    initializingRepoList = true;

    repoDropdown->clear();

    QFile file(getRepoFilePath());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        QString content = stream.readAll();
        file.close();

        QStringList lines = content.split("\n", Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            QStringList parts = line.split("::");
            if (parts.size() == 2) {
                QString repoName = parts[0].trimmed();
                QString repoUrl = parts[1].trimmed();
                repoDropdown->addItem(repoName, repoUrl);
            }
        }
    }

    initializingRepoList = false;

    //auto-load first repo
    if (repoDropdown->count() > 0) {
        onLoadRepoClicked();
    }
}

void AddonRepoManager::showError(const QString& message) {
    QMessageBox::critical(this, "Error", message);
}

void AddonRepoManager::showInfo(const QString& message) {
    QMessageBox::information(this, "Success", message);
}

bool AddonRepoManager::downloadFile(const QString& url, QString& content) {
    QNetworkAccessManager manager;
    QUrl qurl(url);
    QNetworkRequest request(qurl);
    QNetworkReply* reply = manager.get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Download failed for URL: " << url;
        reply->deleteLater();
        return false;
    }

    content = reply->readAll();
    reply->deleteLater();

    return true;
}

QString AddonRepoManager::getRepoFilePath() const {
    return QDir::currentPath() + "/addon_repos.txt";
}

void AddonRepoManager::saveRepoToFile(const QString& name, const QString& url) {
    QFile file(getRepoFilePath());
    QString entry = QString("%1::%2").arg(name, url);

    // Avoid duplicate entries
    QStringList lines;
    if (file.exists()) {
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.startsWith(name + "::")) {
                    lines << line;
                }
            }
            file.close();
        }
    }

    lines << entry;

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString& line : lines) {
            out << line << "\n";
        }
        file.close();
    }
}

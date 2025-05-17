#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QProgressDialog>
#include <QLabel>
#include <QTimer>
#include <QNetworkRequest>
#include <QDesktopServices>
#include <QIcon>
#include <QScrollBar>
#include "addonrepomanager.h"
#include <QTextStream>
#include <QSettings>
#include <QTextBrowser>

// -----------------------------
// MANUAL CONFIGURATION NEEDED

// Customize the values below to match your server's info. Then build the launcher.
// No need to change anything else in the code unless you want to tweak more advanced stuff.
// -----------------------------

// News source - if using multiple languages, make sure to take into account the string before language codes are appended below in fetchNews()
// For example: my news files are called "latest_news_EN" and "latest_news_DE". So I enter the URL as "https://myserver.com/latest_news_" without the 2 letter language code so it can be appended correctly later.
// If you only support 1 language, you can put the full URL here. The language selector box is in the top right corner of the launcher when enabled.
QString newsUrl = "https://myserver.com/latest_news_EN";

// Custom exe
const QString exeUrl = "https://github.com/batman/custom_exe/releases/latest/download/WoW.exe";

// Custom patch
const QString customPatchUrl = "https://github.com/batman/custom_patches/releases/latest/download/patch69.mpq";

// Realmlist - you must have 'set realmlist' here before your server address.
const QString realmlistUrl = "set realmlist logon.hateforge.com";

// Server name for realmlist message "Realmlist already set to X"
const QString serverName = "RaspyWoW";

// Update URL for launcher updates
const QString updateUrl = "https://github.com/batman/launcher/releases/latest/download/QTRaspy.exe";

// Discord invite URL. Make sure it doesn't expire.
const QString discordInviteUrl = "https://discord.gg/6rhp7v2nrg";

// Website URL
const QString websiteUrl = "https://www.raspywow.com/";

// Multi-language support. Make sure to double check the newsUrl variable above if you change this setting.
constexpr bool multiLanguage = false;
const QList<QPair<QString, QString>> supportedLanguages = {
    {"English", "EN"},
    {"Deutsch", "DE"},
    // Add/change languages for the language selector dropdown here.
};

// -----------------------------
// END MANUAL CONFIGURATION
// -----------------------------

// Stop processing hyperlinks as internal documents and blanking news when they are clicked.
class NoNavigateTextBrowser : public QTextBrowser {
public:
    NoNavigateTextBrowser(QWidget *parent = nullptr) : QTextBrowser(parent) {}

protected:
    QVariant loadResource(int type, const QUrl &name) override {
        Q_UNUSED(type);
        Q_UNUSED(name);
        return QVariant();
    }
};

class NewsWidget : public QWidget {
    Q_OBJECT

public:
    NewsWidget(QWidget *parent = nullptr) : QWidget(parent) {
        setFixedSize(640, 480);
        setWindowIcon(QIcon(":/assets/Icon1.ico"));
        if (!newsBg.load(":/assets/news_bg.bmp")) qWarning() << "Failed to load news_bg.bmp";
        if (!logo.load(":/assets/main_bg.bmp")) qWarning() << "Failed to load main_bg.bmp";
        if (!playButton.load(":/assets/button_play.bmp")) qWarning() << "Failed to load button_play.bmp";
        if (!patchButton.load(":/assets/button_patch.bmp")) qWarning() << "Failed to load button_patch.bmp";
        if (!realmlistButton.load(":/assets/button_realmlist.bmp")) qWarning() << "Failed to load button_realmlist.bmp";
        if (!discordButton.load(":/assets/button_discord.bmp")) qWarning() << "Failed to load button_discord.bmp";
        if (!websiteButton.load(":/assets/button_website.bmp")) qWarning() << "Failed to load button_website.bmp";
        if (!exeButton.load(":/assets/button_exe.bmp")) qWarning() << "Failed to load button_exe.bmp";
        if (!updateButton.load(":/assets/button_update.bmp")) qWarning() << "Failed to load button_update.bmp";
        if (!addonButton.load(":/assets/button_addon.bmp")) qWarning() << "Failed to load button_addon.bmp";
        if (!licenseButton.load(":/assets/button_license.bmp")) qWarning() << "Failed to load button_addon.bmp";

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(25, 90, 30, 110);

        newsTimerLabel = new QLabel(this);
        newsTimerLabel->setStyleSheet("font-weight: bold; color: black; background: transparent;");
        newsTimerLabel->move(25, 70);
        newsTimerLabel->resize(400, 20);

        newsTextEdit = new NoNavigateTextBrowser(this);
        newsTextEdit->setOpenExternalLinks(false);
        newsTextEdit->setOpenLinks(false); // used to make news not blank out when clicking links
        newsTextEdit->setReadOnly(true); // used to make news not blank out when clicking links
        newsTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        newsTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        newsTextEdit->setStyleSheet("background: transparent; border: 2px solid black;");
        //newsTextEdit->setFixedSize(300, 280);  // 300px wide and 400px tall (adjust height as needed)
        layout->addWidget(newsTextEdit);

        // Clickable hyperlinks in news window. Used to make news not blank out when clicking links
        connect(newsTextEdit, &QTextBrowser::anchorClicked, this, [](const QUrl &url) {
            QDesktopServices::openUrl(url);
        });

        setMouseTracking(true);
        if (multiLanguage) {
            languageSelector = new QComboBox(this);
            for (const auto& lang : supportedLanguages) {
                languageSelector->addItem(lang.first, lang.second);
            }
            languageSelector->move(600, 0);  // Position in the top-right corner

            // Set smaller dimensions
            languageSelector->setFixedHeight(20);
            languageSelector->setFixedWidth(75);

            // Set custom styling for a more compact look
            languageSelector->setStyleSheet("QComboBox {"
                                            "height: 20px; "
                                            "width: 75px; "
                                            "font-size: 10px; "
                                            "padding: 2px; "
                                            "border: 1px solid #888888; "
                                            "border-radius: 5px; "
                                            "background-color: #b8b4ae; "
                                            //"background-color: #f2f2f2; "
                                            "}");

            // Load the saved language preference
            QSettings settings("QTRaspy", "QTRaspy");
            QString savedLanguage = settings.value("language", "EN").toString();  // Default is "EN"
            languageSelector->setCurrentIndex(languageSelector->findData(savedLanguage));

            // Connect the language selector to update news content when the selection changes
            connect(languageSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int) {
                QString selectedLanguage = languageSelector->currentData().toString();
                QSettings settings("QTRaspy", "QTRaspy");
                settings.setValue("language", selectedLanguage);
                fetchNews();
            });
        }
        fetchNews();
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        pressPos = event->pos();
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        QPoint releasePos = event->pos();

        // Fire button on press and release in same area
        if (buttonRect("patch").contains(pressPos) && buttonRect("patch").contains(releasePos)) {
            handlePatchButton();
        } else if (buttonRect("exe").contains(pressPos) && buttonRect("exe").contains(releasePos)) {
            handleExeButton();
        } else if (buttonRect("realmlist").contains(pressPos) && buttonRect("realmlist").contains(releasePos)) {
            handleRealmlistButton();
        } else if (buttonRect("discord").contains(pressPos) && buttonRect("discord").contains(releasePos)) {
            QDesktopServices::openUrl(QUrl(discordInviteUrl));
        } else if (buttonRect("website").contains(pressPos) && buttonRect("website").contains(releasePos)) {
            QDesktopServices::openUrl(QUrl(websiteUrl));
        } else if (buttonRect("play").contains(pressPos) && buttonRect("play").contains(releasePos)) {
            handlePlayButton();
        } else if (buttonRect("update").contains(pressPos) && buttonRect("update").contains(releasePos)) {
            handleUpdateButton();
        } else if (buttonRect("addon").contains(pressPos) && buttonRect("addon").contains(releasePos)) {
            handleAddonButton();
        } else if (buttonRect("license").contains(pressPos) && buttonRect("license").contains(releasePos)) { //test for license
            handleLicenseButton();
        }

    }

    void mouseMoveEvent(QMouseEvent *event) override {
        hoverPlayButton = buttonRect("play").contains(event->pos());
        hoverPatchButton = buttonRect("patch").contains(event->pos());
        hoverExeButton = buttonRect("exe").contains(event->pos());
        hoverRealmlistButton = buttonRect("realmlist").contains(event->pos());
        hoverDiscordButton = buttonRect("discord").contains(event->pos());
        hoverWebsiteButton = buttonRect("website").contains(event->pos());
        hoverUpdateButton = buttonRect("update").contains(event->pos());
        hoverAddonButton = buttonRect("addon").contains(event->pos());
        hoverLicenseButton = buttonRect("license").contains(event->pos());

        update();  // Trigger a repaint
    }

    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawPixmap(0, 0, newsBg);
        painter.drawPixmap(0, newsBg.height(), logo);

        drawButton(painter, "play", playButton, hoverPlayButton);
        drawButton(painter, "patch", patchButton, hoverPatchButton);
        drawButton(painter, "exe", exeButton, hoverExeButton);
        drawButton(painter, "realmlist", realmlistButton, hoverRealmlistButton);
        drawButton(painter, "discord", discordButton, hoverDiscordButton);
        drawButton(painter, "website", websiteButton, hoverWebsiteButton);
        drawButton(painter, "update", updateButton, hoverUpdateButton);
        drawButton(painter, "addon", addonButton, hoverAddonButton);
        drawButton(painter, "license", licenseButton, hoverLicenseButton);
    }

private:
    QPoint pressPos;
    QPixmap newsBg, logo, playButton, patchButton, realmlistButton, discordButton, websiteButton, exeButton, updateButton, addonButton, licenseButton;
    QString newsText = "Loading latest news...";
    QTextBrowser *newsTextEdit;
    QLabel *newsTimerLabel;
    QTimer *refreshTimer = nullptr;
    const int refreshIntervalSeconds = 30;
    int countdownSeconds = refreshIntervalSeconds;
    QComboBox *languageSelector;

    bool hoverPlayButton = false, hoverPatchButton = false, hoverExeButton = false, hoverUpdateButton = false;
    bool hoverRealmlistButton = false, hoverDiscordButton = false, hoverWebsiteButton = false, hoverAddonButton = false, hoverLicenseButton = false;

    QRect buttonRect(const QString &button) const {
        if (button == "patch") return QRect(363, 438, 103, 25);
        if (button == "exe") return QRect(363, 406, 103, 25);
        if (button == "realmlist") return QRect(247, 438, 103, 25);
        if (button == "discord") return QRect(131, 438, 103, 25);
        if (button == "website") return QRect(15, 438, 103, 25);
        if (button == "play") return QRect(529, 413, 95, 53);
        if (button == "update") return QRect(131, 406, 103, 25);
        if (button == "addon") return QRect(247, 406, 103, 25);
        if (button == "license") return QRect(15, 406, 103, 25);
        return QRect();
    }

    void drawButton(QPainter &painter, const QString &button, const QPixmap &buttonImage, bool isHovered) {
        QPixmap buttonToDraw = buttonImage;

        if (isHovered) {
            buttonToDraw = buttonToDraw.scaled(buttonImage.size(), Qt::KeepAspectRatioByExpanding);

            QImage img = buttonToDraw.toImage();
            QPainter imgPainter(&img);

            imgPainter.setCompositionMode(QPainter::CompositionMode_Screen);

            QRadialGradient gradient(img.width() / 2, img.height() / 2, img.width() / 2);
            gradient.setColorAt(0.0, QColor(100, 255, 200, 100)); // blue-green / teal
            gradient.setColorAt(1.0, QColor(100, 255, 200, 0));   // transparent at edges

            imgPainter.fillRect(img.rect(), gradient);

            imgPainter.end();

            buttonToDraw = QPixmap::fromImage(img);
        }

        painter.drawPixmap(buttonRect(button).topLeft(), buttonToDraw);
    }

    void fetchNews() {
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply) {
            if (reply->error() == QNetworkReply::NoError) {
                QString newText = QString::fromUtf8(reply->readAll()).trimmed();
                if (newText != newsText) {
                    newsText = newText;
                    newsTextEdit->setHtml(newsText);
                    newsTextEdit->verticalScrollBar()->setValue(0); // Snap to top if new content
                } else {
                    int scrollPos = newsTextEdit->verticalScrollBar()->value();
                    newsTextEdit->setHtml(newsText);
                    newsTextEdit->verticalScrollBar()->setValue(scrollPos); // Keep position if no change
                }
                startCountdown(true);
            } else {
                newsText = "Failed to load news. Check your internet connection or check Discord for the latest updates.";
                newsTextEdit->setHtml(newsText);
                startCountdown(false);
            }
            reply->deleteLater();
        });

        // Final news URL parsing.
        QString newsUrlFinal = newsUrl;
        if (multiLanguage) {
            if (languageSelector) {
                QString langCode = languageSelector->currentData().toString();
                if (!langCode.isEmpty()) {
                     newsUrlFinal += langCode;
                }
            }
        }
        manager->get(QNetworkRequest(QUrl(newsUrlFinal)));
    }

    void startCountdown(bool success) {
        if (refreshTimer) {
            refreshTimer->stop();
            refreshTimer->deleteLater();
        }

        countdownSeconds = refreshIntervalSeconds;
        refreshTimer = new QTimer(this);

        connect(refreshTimer, &QTimer::timeout, this, [=]() {
            countdownSeconds--;
            if (countdownSeconds <= 0) {
                refreshTimer->stop();
                fetchNews();
            } else {
                if (success) {
                    newsTimerLabel->setText(QString("Refreshing news in %1 seconds").arg(countdownSeconds));
                } else {
                    newsTimerLabel->setText(QString("Retrying news fetch in %1 seconds").arg(countdownSeconds));
                }
            }
        });

        refreshTimer->start(1000);
        if (success) {
            newsTimerLabel->setText(QString("Refreshing news in %1 seconds").arg(countdownSeconds));
        } else {
            newsTimerLabel->setText(QString("Retrying news fetch in %1 seconds").arg(countdownSeconds));
        }
    }

    void handlePatchButton();
    void handleExeButton();
    void handleRealmlistButton();
    void handlePlayButton();
    void handleUpdateButton();
    void handleAddonButton();
    void handleLicenseButton();
};

void NewsWidget::handlePatchButton() {
    QString baseDir = QDir::currentPath();
    QString dataDir = baseDir + "/Data";
    QString wdbPath = baseDir + "/WDB";
    QString patchPath = dataDir + "/patch-X.mpq";
    QString patchOldPath = dataDir + "/patch-X.mpq.OLD";
    QString downloadUrl = customPatchUrl;

    QProgressDialog *progressDialog = new QProgressDialog("Downloading Custom Patch and clearing WDB...", "Cancel", 0, 0, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setWindowTitle("Please wait");
    progressDialog->setCancelButton(nullptr);
    progressDialog->show();

    QTimer::singleShot(100, this, [=]() {
        QDir wdbDir(wdbPath);
        if (wdbDir.exists()) {
            wdbDir.removeRecursively();
        }
        QDir().mkpath(dataDir);

        if (QFile::exists(patchPath)) {
            QFile::remove(patchOldPath);
            QFile::rename(patchPath, patchOldPath);
        }

        QNetworkAccessManager *downloadManager = new QNetworkAccessManager(this);
        QNetworkReply *reply = downloadManager->get(QNetworkRequest(QUrl(downloadUrl)));
        QFile *outFile = new QFile(patchPath);

        if (!outFile->open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Error", "Failed to open patch file for writing.");
            progressDialog->close();
            return;
        }

        connect(reply, &QNetworkReply::readyRead, this, [=]() {
            outFile->write(reply->readAll());
        });

        connect(reply, &QNetworkReply::finished, this, [=]() {
            progressDialog->close();
            if (reply->error() != QNetworkReply::NoError) {
                outFile->close();
                outFile->remove();
                if (QFile::exists(patchOldPath)) {
                    QFile::rename(patchOldPath, patchPath);
                }
                QMessageBox::critical(this, "Download Failed",
                                      "The patch cannot be downloaded. Check your connection or try again later.");
                reply->deleteLater();
                return;
            }

            outFile->flush();
            outFile->close();
            QFile::remove(patchOldPath);
            reply->deleteLater();

            QMessageBox::information(this, "Custom Patch Complete",
                                     "Custom patch updated. To revert, remove patch-X.mpq from Data and delete WDB.");
        });
    });
}

void NewsWidget::handleExeButton() {
    QString baseDir = QDir::currentPath();
    QString oldFilesDir = baseDir + "/original backups";
    QString currentExe = baseDir + "/WoW.exe";
    QString tempBackup = baseDir + "/WoW.exe.TEMP";
    QString backupExe = oldFilesDir + "/WoW.exe";
    QString downloadUrl = exeUrl;

    QMessageBox confirmBox;
    confirmBox.setWindowTitle("Confirm WoW.exe Replacement");
    confirmBox.setTextFormat(Qt::RichText);
    confirmBox.setText(R"(
    <p>This will download a new <b>WoW.exe</b> file that has been modified with Vanilla Tweaks:<br>
    <a href='https://github.com/brndd/vanilla-tweaks'>https://github.com/brndd/vanilla-tweaks</a></p>
    <p>The hash of our specific version is whitelisted to play on the server. Our version contains the following features:</p>
    <ul>
        <li>Autoloot</li>
        <li>4GB memory patch (less crashing and more addons!)</li>
        <li>More audio channels (better quality audio)</li>
        <li>Nameplate distance increased</li>
        <li>Sound in background when alt-tabbed</li>
        <li>Max camera distance increased</li>
        <li>Slightly higher render distance</li>
        <li>Widescreen FOV fix</li>
    </ul>
    <p><b>Are you sure you want to replace your current WoW.exe file?</b><br>
    It will be backed up to the <code>original backups</code> folder in your game directory (only if not already present).</p>
)");

    confirmBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    confirmBox.setDefaultButton(QMessageBox::Cancel);
    confirmBox.setIcon(QMessageBox::Warning);
    confirmBox.setTextInteractionFlags(Qt::TextBrowserInteraction);

    if (confirmBox.exec() != QMessageBox::Yes) return;

    QProgressDialog *progressDialog = new QProgressDialog("Downloading custom WoW.exe...", "Cancel", 0, 0, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setWindowTitle("Please wait");
    progressDialog->setCancelButton(nullptr);
    progressDialog->show();

    QTimer::singleShot(100, this, [=]() {
        QDir().mkpath(oldFilesDir);

        if (QFile::exists(currentExe)) {
            if (!QFile::rename(currentExe, tempBackup)) {
                QMessageBox::critical(this, "Error", "Could not backup WoW.exe. Make sure it isn't running.");
                progressDialog->close();
                return;
            }
        }

        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(downloadUrl)));
        QFile *outFile = new QFile(currentExe);

        if (!outFile->open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Error", "Failed to open WoW.exe for writing.");
            if (QFile::exists(tempBackup)) {
                QFile::rename(tempBackup, currentExe);
            }
            progressDialog->close();
            return;
        }

        connect(reply, &QNetworkReply::readyRead, this, [=]() {
            outFile->write(reply->readAll());
        });

        connect(reply, &QNetworkReply::finished, this, [=]() {
            outFile->flush();
            outFile->close();
            progressDialog->close();

            if (reply->error() != QNetworkReply::NoError) {
                outFile->remove();
                if (QFile::exists(tempBackup)) {
                    QFile::rename(tempBackup, currentExe);
                }
                QMessageBox::critical(this, "Download Failed",
                                      "The custom WoW.exe could not be downloaded. Your original was restored.");
                reply->deleteLater();
                return;
            }

            if (!QFile::exists(backupExe)) {
                QFile::rename(tempBackup, backupExe);
            } else {
                QFile::remove(tempBackup);
            }

            QMessageBox::information(this, "Custom WoW.exe Installed",
                                     "Custom WoW.exe installed successfully. Backup saved in \"original backups\".");
            reply->deleteLater();
        });
    });
}

void NewsWidget::handleRealmlistButton() {
    QString baseDir = QDir::currentPath();
    QString realmlistPath = baseDir + "/realmlist.wtf";
    QString oldFilesDir = baseDir + "/original backups";
    QString backupRealmlist = oldFilesDir + "/realmlist.wtf";
    QString targetText = realmlistUrl;

    QDir().mkpath(oldFilesDir);

    QFile currentRealmlist(realmlistPath);

    bool needsUpdate = true;
    if (currentRealmlist.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = QString::fromUtf8(currentRealmlist.readAll()).trimmed();
        currentRealmlist.close();

        if (content == targetText) {
            QMessageBox::information(this, "Realmlist already set", QString("Already set to %1.").arg(serverName));

            return;
        }
    }

    if (!QFile::exists(backupRealmlist) && currentRealmlist.exists()) {
        currentRealmlist.copy(backupRealmlist);
    }

    if (currentRealmlist.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&currentRealmlist);
        out << targetText << "\n";
        currentRealmlist.close();
        QMessageBox::information(this, "Realmlist Set", "Realmlist set successfully.");
    } else {
        QMessageBox::critical(this, "Error", "Could not write realmlist.wtf.");
    }
}

void NewsWidget::handleUpdateButton() {
    QString baseDir = QDir::currentPath();
    QString currentLauncher = baseDir + "/QTRaspy.exe";
    QString backupLauncher = baseDir + "/QTRaspy_backup.exe";
    QString tempLauncher = baseDir + "/QTRaspy_new.exe";

    QString downloadUrl = updateUrl;

    QProgressDialog *progressDialog = new QProgressDialog("Updating Launcher...", "Cancel", 0, 0, this);
    progressDialog->setWindowModality(Qt::WindowModal);
    progressDialog->setWindowTitle("Updating");
    progressDialog->setCancelButton(nullptr);
    progressDialog->show();

    QTimer::singleShot(100, this, [=]() {
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(downloadUrl)));
        QFile *outFile = new QFile(tempLauncher);

        if (!outFile->open(QIODevice::WriteOnly)) {
            QMessageBox::critical(this, "Error", "Failed to open temp file for writing.");
            progressDialog->close();
            return;
        }

        connect(reply, &QNetworkReply::readyRead, this, [=]() {
            outFile->write(reply->readAll());
        });

        connect(reply, &QNetworkReply::finished, this, [=]() {
            outFile->flush();
            outFile->close();
            progressDialog->close();

            if (reply->error() != QNetworkReply::NoError) {
                outFile->remove();
                QMessageBox::critical(this, "Update Failed", "Failed to download new launcher.");
                reply->deleteLater();
                return;
            }

            // Backup current launcher before replacing
            if (QFile::exists(backupLauncher)) {
                QFile::remove(backupLauncher);
            }
            if (QFile::exists(currentLauncher)) {
                QFile::rename(currentLauncher, backupLauncher);
            }

            // Replace with the new launcher
            if (!QFile::rename(tempLauncher, currentLauncher)) {
                QMessageBox::critical(this, "Error", "Failed to replace Launcher.exe.");
                return;
            }

            QMessageBox::information(this, "Update Complete", "Launcher updated successfully. Restarting...");
            QProcess::startDetached(currentLauncher);
            qApp->quit();  // Quit current launcher
            reply->deleteLater();
        });
    });
}

void NewsWidget::handleAddonButton() {
    AddonRepoManager* repoManager = new AddonRepoManager(this);
    repoManager->exec();
}

void NewsWidget::handlePlayButton() {
    QString baseDir = QDir::currentPath();
    QString vanillaFixesPath = baseDir + "/VanillaFixes.exe";
    QString wowPath = baseDir + "/WoW.exe";

    QString programToRun;
    if (QFile::exists(vanillaFixesPath)) {
        programToRun = vanillaFixesPath;
    } else if (QFile::exists(wowPath)) {
        programToRun = wowPath;
    } else {
        QMessageBox::critical(this, "Executable Not Found",
                              "Neither VanillaFixes.exe nor WoW.exe found. Make sure the QTRaspy launcher is in your WoW folder.");
        return;
    }

    QProcess::startDetached(programToRun);
    close();
}

void NewsWidget::handleLicenseButton() {
    // Create a custom dialog
    QDialog *licenseDialog = new QDialog(this);
    licenseDialog->setWindowTitle("Licenses");
    QTextBrowser *licenseTextBrowser = new QTextBrowser(licenseDialog);
    QString licenseInfo =
        "<h2>License Information</h2>"
        "<p>This software is licensed under the GNU General Public License v3.0."
        "<p>It is built on Qt, which is licensed under the GNU LGPL 3.0. It also makes use of the <a href=\"https://github.com/zlib-ng/minizip-ng\">minizip-ng library.</a></p>"
        "<p>For full details, visit the official LGPL 3.0 license at the following link: <a href=\"https://www.gnu.org/licenses/lgpl-3.0.en.html#license-text\">GPL 3.0 License</a></p>"
        "<p>The source code of this launcher is available on the <a href=\"https://github.com/lukzak/Raspy-launcher-latest-test\">QTRaspy Github repo.</a></p>";

    licenseTextBrowser->setHtml(licenseInfo);
    licenseTextBrowser->setOpenExternalLinks(true);

    QPushButton *closeButton = new QPushButton("Close", licenseDialog);
    connect(closeButton, &QPushButton::clicked, licenseDialog, &QDialog::accept);

    QVBoxLayout *layout = new QVBoxLayout(licenseDialog);
    layout->addWidget(licenseTextBrowser);
    layout->addWidget(closeButton);

    licenseDialog->setLayout(layout);
    licenseDialog->exec();
}

#include "main.moc"

int main(int argc, char *argv[]) {
    QString baseDir = QDir::currentPath();
    QString backupLauncher = baseDir + "/QTRaspy_backup.exe";

    if (QFile::exists(backupLauncher)) {
        QFile::remove(backupLauncher);
    }
    QApplication app(argc, argv);
    NewsWidget window;
    window.show();
    return app.exec();
}

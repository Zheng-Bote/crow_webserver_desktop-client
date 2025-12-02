#pragma once

#include <QApplication>
#include <QDesktopServices>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QTextEdit>
#include <QUrl>

#include "includes/rz_config.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoginClicked();
    void onBrowseClicked();
    void onUploadClicked();
    void onNetworkFinished(QNetworkReply *reply);
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void openGithub();

private:
    QSettings *settings;
    QString SERVER_URL = "http://localhost:8080";
    QPushButton *m_statusMiddle;
    QMenu *appMenu;
    QAction *configAct;
    QAction *aboutAct;
    void createMenu();
    void appConfig();
    void appAbout();

    // GUI Elements
    QLineEdit *m_userEdit;
    QLineEdit *m_passEdit;
    QPushButton *m_loginBtn;

    QLineEdit *m_filePathEdit;
    QLineEdit *m_serverPathEdit;
    QPushButton *m_browseBtn;
    QPushButton *m_uploadBtn;

    QTextEdit *m_logArea;
    QLabel *m_statusLabel;

    QProgressBar *m_progressBar;

    // Networking
    QNetworkAccessManager *m_netManager;
    QString m_jwtToken;
    QString m_refreshToken;
    bool m_isRefreshing = false;

    void performTokenRefresh();

    // Helper
    void log(const QString &msg);
    void retryLastUpload();
};

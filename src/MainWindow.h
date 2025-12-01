#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onLoginClicked();
    void onBrowseClicked();
    void onUploadClicked();
    void onNetworkFinished(QNetworkReply* reply);

private:
    // GUI Elements
    QLineEdit* m_userEdit;
    QLineEdit* m_passEdit;
    QPushButton* m_loginBtn;

    QLineEdit* m_filePathEdit;
    QLineEdit* m_serverPathEdit;
    QPushButton* m_browseBtn;
    QPushButton* m_uploadBtn;

    QTextEdit* m_logArea;
    QLabel* m_statusLabel;

    // Networking
    QNetworkAccessManager* m_netManager;
    QString m_jwtToken; // Hier speichern wir den Token
    
    // Helper
    void log(const QString& msg);
};

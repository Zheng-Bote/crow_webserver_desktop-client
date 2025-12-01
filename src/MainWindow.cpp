#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QHttpMultiPart>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

// Konfiguration (Anpassen falls Server woanders läuft)
const QString SERVER_URL = "http://localhost:8080";

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // --- 1. Login Sektion ---
    QGroupBox* loginGroup = new QGroupBox("1. Authentifizierung", this);
    QFormLayout* loginLayout = new QFormLayout(loginGroup);
    
    m_userEdit = new QLineEdit("admin", this);
    m_passEdit = new QLineEdit("1234", this); // Standard Passwort aus dem Server-Code
    m_passEdit->setEchoMode(QLineEdit::Password);
    m_loginBtn = new QPushButton("Login", this);
    
    loginLayout->addRow("Username:", m_userEdit);
    loginLayout->addRow("Password:", m_passEdit);
    loginLayout->addRow("", m_loginBtn);
    
    mainLayout->addWidget(loginGroup);

    // --- 2. Upload Sektion ---
    QGroupBox* uploadGroup = new QGroupBox("2. Photo Upload", this);
    QVBoxLayout* uploadLayout = new QVBoxLayout(uploadGroup);

    // Datei Auswahl
    QHBoxLayout* fileLayout = new QHBoxLayout();
    m_filePathEdit = new QLineEdit(this);
    m_filePathEdit->setPlaceholderText("Bitte Datei auswählen...");
    m_filePathEdit->setReadOnly(true);
    m_browseBtn = new QPushButton("Browse...", this);
    fileLayout->addWidget(m_filePathEdit);
    fileLayout->addWidget(m_browseBtn);

    // Server Pfad
    QHBoxLayout* pathLayout = new QHBoxLayout();
    QLabel* pathLabel = new QLabel("Zielordner (Server):", this);
    m_serverPathEdit = new QLineEdit(this);
    m_serverPathEdit->setPlaceholderText("z.B. urlaub/2024 (optional)");
    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(m_serverPathEdit);

    m_uploadBtn = new QPushButton("Upload Photo", this);
    m_uploadBtn->setEnabled(false); // Erst nach Login aktiv

    uploadLayout->addLayout(fileLayout);
    uploadLayout->addLayout(pathLayout);
    uploadLayout->addWidget(m_uploadBtn);

    mainLayout->addWidget(uploadGroup);

    // --- 3. Log Bereich ---
    m_logArea = new QTextEdit(this);
    m_logArea->setReadOnly(true);
    mainLayout->addWidget(m_logArea);

    // --- Networking Init ---
    m_netManager = new QNetworkAccessManager(this);
    
    // --- Signals & Slots ---
    connect(m_loginBtn, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    connect(m_browseBtn, &QPushButton::clicked, this, &MainWindow::onBrowseClicked);
    connect(m_uploadBtn, &QPushButton::clicked, this, &MainWindow::onUploadClicked);
    
    // Zentraler Handler für alle Antworten
    connect(m_netManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkFinished);

    setWindowTitle("Crow Server Client");
    resize(400, 500);
}

MainWindow::~MainWindow() {}

void MainWindow::log(const QString& msg) {
    m_logArea->append(msg);
}

// --- Logik: Login ---
void MainWindow::onLoginClicked() {
    QUrl url(SERVER_URL + "/login");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // JSON Body bauen
    QJsonObject json;
    json["username"] = m_userEdit->text();
    json["password"] = m_passEdit->text();
    QJsonDocument doc(json);

    log("Logging in...");
    m_netManager->post(request, doc.toJson());
}

// --- Logik: Datei wählen ---
void MainWindow::onBrowseClicked() {
    QString fileName = QFileDialog::getOpenFileName(this, "Bild auswählen", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!fileName.isEmpty()) {
        m_filePathEdit->setText(fileName);
    }
}

// --- Logik: Upload ---
void MainWindow::onUploadClicked() {
    QString filePath = m_filePathEdit->text();
    if (filePath.isEmpty()) {
        QMessageBox::warning(this, "Fehler", "Keine Datei ausgewählt.");
        return;
    }

    QFile* file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        log("Error: Could not open file locally.");
        delete file;
        return;
    }

    QUrl url(SERVER_URL + "/upload");
    QNetworkRequest request(url);
    
    // WICHTIG: Token setzen
    QString bearer = "Bearer " + m_jwtToken;
    request.setRawHeader("Authorization", bearer.toUtf8());

    // --- Multipart Zusammenbau ---
    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // 1. Part: Das Bild
    QHttpPart imagePart;
    // Header für Dateiname und Feldname "photo"
    QString fileName = QFileInfo(filePath).fileName();
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                        QVariant("form-data; name=\"photo\"; filename=\"" + fileName + "\""));
    // Content-Type raten (einfachheitshalber image/jpeg, oder basierend auf Extension)
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart.setBodyDevice(file);
    // Das File wird nun Kind vom MultiPart -> wird automatisch gelöscht
    file->setParent(multiPart); 
    multiPart->append(imagePart);

    // 2. Part: Der Zielpfad (falls angegeben)
    if (!m_serverPathEdit->text().isEmpty()) {
        QHttpPart pathPart;
        pathPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"path\""));
        pathPart.setBody(m_serverPathEdit->text().toUtf8());
        multiPart->append(pathPart);
    }

    log("Uploading file...");
    QNetworkReply* reply = m_netManager->post(request, multiPart);
    
    // MultiPart muss gelöscht werden, wenn der Reply fertig ist
    multiPart->setParent(reply); 
}

// --- Netzwerk Antwort Handler ---
void MainWindow::onNetworkFinished(QNetworkReply* reply) {
    // Reply automatisch löschen später
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        log("Network Error: " + reply->errorString());
        // Server Fehler lesen (z.B. 401 Body)
        log("Server Response: " + reply->readAll());
        return;
    }

    QByteArray responseData = reply->readAll();
    log("Response: " + responseData);

    // Wir schauen, welche URL aufgerufen wurde, um zu wissen was zu tun ist
    QString path = reply->request().url().path();

    if (path == "/login") {
        // Login Parsing
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isObject() && doc.object().contains("token")) {
            m_jwtToken = doc.object()["token"].toString();
            log("Login Success! Token received.");
            m_uploadBtn->setEnabled(true);
            m_statusLabel = new QLabel("Logged In as " + m_userEdit->text());
        } else {
            log("Login Failed: Invalid JSON response.");
        }
    } 
    else if (path == "/upload") {
        log("Upload finished successfully!");
        QMessageBox::information(this, "Success", "Upload erfolgreich!\n" + responseData);
    }
}

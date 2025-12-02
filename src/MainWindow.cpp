#include "MainWindow.h"
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHttpMultiPart>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QVBoxLayout>

// Konfiguration (Anpassen falls Server woanders läuft)
//const QString SERVER_URL = "http://localhost:8080";

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

    // --- 1. Login Sektion ---
    QGroupBox *loginGroup = new QGroupBox(tr("1. Authentification"), this);
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
    m_filePathEdit->setPlaceholderText(tr("Please choose an image file..."));
    m_filePathEdit->setReadOnly(true);
    m_browseBtn = new QPushButton("Browse...", this);
    fileLayout->addWidget(m_filePathEdit);
    fileLayout->addWidget(m_browseBtn);

    // Server Pfad
    QHBoxLayout* pathLayout = new QHBoxLayout();
    QLabel *pathLabel = new QLabel(tr("Target folder (Server):"), this);
    m_serverPathEdit = new QLineEdit(this);
    m_serverPathEdit->setPlaceholderText(tr("e.g. holidays/2025 (optional)"));
    m_serverPathEdit->setToolTip(tr("e.g. holidays/2025 (optional)"));
    pathLayout->addWidget(pathLabel);
    pathLayout->addWidget(m_serverPathEdit);

    m_uploadBtn = new QPushButton("Upload Photo", this);
    m_uploadBtn->setEnabled(false); // Erst nach Login aktiv

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    // Optional: Standardmäßig ausblenden oder auf 0 lassen
    // m_progressBar->setVisible(false);

    uploadLayout->addLayout(fileLayout);
    uploadLayout->addLayout(pathLayout);
    uploadLayout->addWidget(m_uploadBtn);
    uploadLayout->addWidget(m_progressBar);

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

    QString version = "v";
    version.append(PROJECT_VERSION.c_str());
    m_statusMiddle = new QPushButton(version, this);
    m_statusMiddle->setStyleSheet("font-size: 10px;"
                                  "border: none;");
    m_statusMiddle->setToolTip(
        tr("click to open your default Browser and go to the Github repository"));
    statusBar()->addWidget(m_statusMiddle, 1);
    connect(m_statusMiddle, SIGNAL(clicked(bool)), this, SLOT(openGithub()));

    resize(400, 500);

    settings = new QSettings;
    SERVER_URL = settings->contains("Server") ? settings->value("Server").toString() : SERVER_URL;

    createMenu();
}

MainWindow::~MainWindow() {}

void MainWindow::log(const QString& msg) {
    m_logArea->append(msg);
}

void MainWindow::retryLastUpload()
{
    log("Retrying upload with new token...");

    // Sicherheitscheck: Ist überhaupt noch ein Pfad ausgewählt?
    if (m_filePathEdit->text().isEmpty()) {
        log("Retry failed: No file selected anymore.");
        return;
    }

    // Wir rufen einfach die existierende Upload-Logik erneut auf.
    // Diese baut den MultiPart-Request neu zusammen (inkl. neuem Token)
    // und sendet ihn ab.
    onUploadClicked();
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
    QString imagePath = settings->contains("imagePath") ? settings->value("imagePath").toString()
                                                        : QDir::homePath();

    QString fileName
        = QFileDialog::getOpenFileName(this,
                                       tr("choose Image"),
                                       imagePath,
                                       "Images (*.png *.jpg *.jpeg *.bmp *.tiff *.gif)");
    if (!fileName.isEmpty()) {
        m_filePathEdit->setText(fileName);

        QFileInfo fileInfo(fileName);
        m_serverPathEdit->setText(fileInfo.absolutePath());
        settings->setValue("imagePath", fileInfo.absolutePath());
    }
    m_progressBar->setValue(0);
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
    // ProgressBar zurücksetzen
    m_progressBar->setValue(0);

    QNetworkReply *reply = m_netManager->post(request, multiPart);

    // MultiPart muss gelöscht werden, wenn der Reply fertig ist
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::uploadProgress, this, &MainWindow::onUploadProgress);
}

// --- Netzwerk Antwort Handler ---
void MainWindow::onNetworkFinished(QNetworkReply *reply)
{
    // 1. Meta-Daten holen (URL und HTTP Status Code)
    QString path = reply->request().url().path();
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    // ------------------------------------------------------------------
    // FALL 1: Token abgelaufen (401) bei normalen Aktionen (z.B. Upload)
    // ------------------------------------------------------------------
    if (statusCode == 401 && path != "/login" && path != "/refresh") {
        reply->deleteLater(); // Antwort verwerfen

        if (!m_refreshToken.isEmpty()) {
            log("Access Token expired (401). Trying Refresh...");
            performTokenRefresh();
        } else {
            log("Session expired. Please login again.");
            m_uploadBtn->setEnabled(false);
            m_progressBar->setValue(0);
        }
        return; // Wir brechen hier ab, wir lesen keine Daten
    }

    // ------------------------------------------------------------------
    // Alle anderen Fälle: Wir lesen die Antwortdaten
    // ------------------------------------------------------------------
    QByteArray responseData = reply->readAll(); // <--- HIER wird responseData definiert
    reply->deleteLater();

    // ------------------------------------------------------------------
    // FALL 2: Refresh Token Antwort (/refresh)
    // ------------------------------------------------------------------
    if (path == "/refresh") {
        m_isRefreshing = false; // Flag zurücksetzen

        if (statusCode == 200) {
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (doc.isObject() && doc.object().contains("token")) {
                m_jwtToken = doc.object()["token"].toString();
                log("Token refreshed successfully. Retrying last action...");

                // Aktion wiederholen (Hier hardcoded auf Upload,
                // in komplexen Apps würde man das Request-Objekt speichern)
                //onUploadClicked();
                retryLastUpload();
            }
        } else {
            log("Refresh failed (Session invalid). Please login again.");
            m_jwtToken.clear();
            m_refreshToken.clear();
            m_uploadBtn->setEnabled(false);
        }
        return;
    }

    // ------------------------------------------------------------------
    // FALL 3: Allgemeine Netzwerkfehler (außer 401, das haben wir oben behandelt)
    // ------------------------------------------------------------------
    if (reply->error() != QNetworkReply::NoError) {
        m_progressBar->setValue(0);
        log("Network Error: " + reply->errorString());
        log("Server Message: " + responseData); // Hier nutzen wir responseData
        return;
    }

    // ------------------------------------------------------------------
    // FALL 4: Normale Erfolgsfälle (/login, /upload)
    // ------------------------------------------------------------------
    if (path == "/login") {
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("token") && obj.contains("refreshToken")) {
                m_jwtToken = obj["token"].toString();
                m_refreshToken = obj["refreshToken"].toString();

                log("Login Success! Tokens received.");
                m_uploadBtn->setEnabled(true);
                // Status Label update etc.
            } else {
                log("Login failed: Invalid JSON response.");
            }
        }
    } else if (path == "/upload") {
        m_progressBar->setValue(100);
        log("Upload finished successfully!");
        // Optional: responseData anzeigen
        // log("Server: " + responseData);
    }
}

void MainWindow::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    // bytesTotal kann -1 sein, wenn die Größe unbekannt ist
    if (bytesTotal > 0) {
        int percent = static_cast<int>((bytesSent * 100) / bytesTotal);
        m_progressBar->setValue(percent);

        // Optional: Log update nur alle 10% um Spam zu vermeiden,
        // oder einfach Statusbar updaten
        // m_statusLabel->setText(QString("Upload: %1%").arg(percent));
    }
}

void MainWindow::openGithub()
{
    QDesktopServices::openUrl(QUrl(PROJECT_HOMEPAGE_URL.c_str()));
}

void MainWindow::createMenu()
{
    menuBar()->setNativeMenuBar(false);
#ifdef Q_OS_MACOS
    menuBar()->setLayoutDirection(Qt::LayoutDirection::RightToLeft);
#endif

    aboutAct = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::HelpAbout), tr("&About"), this);
    aboutAct->setShortcuts(QKeySequence::WhatsThis);
    connect(aboutAct, &QAction::triggered, this, &MainWindow::appAbout);

    configAct = new QAction(QIcon::fromTheme(QIcon::ThemeIcon::DocumentProperties),
                            tr("&Config"),
                            this);
    connect(configAct, &QAction::triggered, this, &MainWindow::appConfig);

    appMenu = menuBar()->addMenu(tr("&System"));
    appMenu->addAction(aboutAct);
    appMenu->addAction(configAct);
}

void MainWindow::appConfig()
{
    bool ok;

    QString text = QInputDialog::getText(this,
                                         tr("Server URL"),
                                         tr("Please enter the new Server URL"),
                                         QLineEdit::Normal,
                                         SERVER_URL,
                                         &ok);
    if (ok && !text.isEmpty()) {
        settings->setValue("Server", text);
        SERVER_URL = text;
    }
}

void MainWindow::appAbout()
{
    QString text = "<p><b>";
    text.append(PROG_LONGNAME.c_str());
    text.append("</b></p>");

    QString setInformativeText = "<p>";
    setInformativeText.append(PROJECT_NAME.c_str());
    setInformativeText.append(" v");
    setInformativeText.append(PROJECT_VERSION);
    setInformativeText.append("</p><p>");
    setInformativeText.append(PROJECT_DESCRIPTION);
    setInformativeText.append("</p><p>Copyright (&copy;) ");
    setInformativeText.append(PROG_CREATED);
    setInformativeText.append(" " + PROG_AUTHOR);
    setInformativeText.append("</p><p><a href=\"");
    setInformativeText.append(PROJECT_HOMEPAGE_URL);
    setInformativeText.append("\" alt=\"Github Repository\">Github Repository</a></p>");

    QString cmake_info = "<p>Compiled with:<br/>";
    cmake_info.append(CMAKE_CXX_COMPILER + " " + CMAKE_CXX_STANDARD + " QT " + CMAKE_QT_VERSION
                      + "</p>");
    setInformativeText.append(cmake_info);

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(tr("About"));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setTextFormat(Qt::RichText);
    msgBox.setText(text);
    msgBox.setInformativeText(setInformativeText);
    //msgBox.setFixedWidth(900);
    msgBox.exec();
}

void MainWindow::performTokenRefresh()
{
    if (m_isRefreshing)
        return;
    m_isRefreshing = true;

    QUrl url(SERVER_URL + "/refresh");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["refreshToken"] = m_refreshToken;

    log("Token expired. Attempting refresh...");
    m_netManager->post(request, QJsonDocument(json).toJson());
}

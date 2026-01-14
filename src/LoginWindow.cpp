#include "LoginWindow.hpp"
#include "ui_LoginWindow.h"
#include "managerwindow.h"
#include "mainwindow.h"
#include <QGraphicsDropShadowEffect>
#include<QMessageBox>


LoginWindow::LoginWindow(QWidget *parent, Services* services)
 : QDialog(parent), ui(new Ui::LoginWindow),services_(services)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/resources/icons/Bank of World.ico"));

    this->setStyleSheet(R"(
    QDialog {
        background-color: #f2f4f8;
        font-family: 'Segoe UI';
    }

    /* Card container */
    QFrame#cardFrame {
        background: white;
        border-radius: 16px;
    }

    /* Title */
    QLabel#titleLabel {
        font-size: 18pt;
        font-weight: 700;
        color: #0D3B66;
        padding:20px;
    }

    /* Inputs */
    QLineEdit {
        border: 1px solid #dcdde1;
        border-radius: 10px;
        padding: 6px 10px;
        font-size: 10pt;

    }

    QLineEdit:focus {
        border: 1px solid #0D3B66;
    }

    /* Button */
    QPushButton#loginButton {
        background-color: #0D3B66;
        color: white;
        border-radius: 10px;
        padding: 6px;
        font-size: 10pt;
        font-weight: 600;
    }

    QPushButton#loginButton:hover {
        background-color: #092A4C;
    }

    QPushButton#loginButton:pressed {
        background-color: #061C33;
    }

    QMessageBox {
        background-color: #ffffff;
    }

    QMessageBox QLabel {
        color: #1e1e1e;   /* dark readable text */
        font-size: 10pt;
    }

    QMessageBox QPushButton {
        background-color: #0D3B66;
        color: white;
        border-radius: 6px;
        padding: 6px 14px;
    }

    QMessageBox QPushButton:hover {
        background-color: #092A4C;
    }

    QLabel#sloganLabel {
        font-size: 9.5pt;
        color: #4b5563;       /* darker gray */
        letter-spacing: 0.5px;
        margin-top: -4px;   /* pulls it closer to title */
        margin-bottom: 14px;
    }

    QLabel#signupHintLabel {
        color: #6b7280;
        font-size: 9.5pt;
    }

    )");

    ui->titleLabel->setAlignment(Qt::AlignCenter);
    this->setWindowTitle("Bank of World");
    ui->usernameLineEdit->setPlaceholderText("User ID");
    ui->passwordLineEdit->setPlaceholderText("Password");
    ui->passwordLineEdit->setEchoMode(QLineEdit::Password);
    connect(ui->loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(35);
    shadow->setOffset(0, 12);
    shadow->setColor(QColor(0, 0, 0, 80));
    ui->cardFrame->setGraphicsEffect(shadow);
    ui->usernameLineEdit->setMinimumHeight(32);
    ui->passwordLineEdit->setMinimumHeight(32);
    ui->loginButton->setMinimumHeight(36);

    // Press Enter to login
    ui->loginButton->setDefault(true);
    ui->loginButton->setAutoDefault(true);

    // Cursor hint
    ui->loginButton->setCursor(Qt::PointingHandCursor);
    ui->centralWidget->layout()->activate();
    ui->cardFrame->adjustSize();
    this->adjustSize();
    this->setFixedSize(this->sizeHint());
    this->setFixedSize(500, 500);






}

LoginWindow::~LoginWindow()
{
    delete ui;
}

void LoginWindow::handleLogin()
{
    const QString userQ = ui->usernameLineEdit->text().trimmed();
    const QString passQ = ui->passwordLineEdit->text();

    if (userQ.isEmpty() || passQ.isEmpty()) {
        QMessageBox::warning(this, "Login Failed", "Please enter User ID and Password.");
        return;
    }

    if (!services_) {
        QMessageBox::critical(this, "Error", "services_ is null");
        return;
    }

    std::shared_ptr<Customers> customer;
    Services::LoginRole role = services_->authenticate(userQ.toStdString(), passQ.toStdString(), customer);

    if (role == Services::LoginRole::Manager) {
        auto *mw = new ManagerWindow(nullptr, services_);
        mw->setAttribute(Qt::WA_DeleteOnClose);
        mw->show();
        this->close();
        return;
    }

    if (role == Services::LoginRole::Customer) {
        if (!customer) {
            QMessageBox::warning(this, "Login Failed", "Customer session invalid.");
            return;
        }
        auto *cw = new MainWindow(nullptr, services_, customer);
        cw->setAttribute(Qt::WA_DeleteOnClose);
        cw->show();
        this->close();
        return;
    }

    QMessageBox::warning(this, "Login Failed", "Incorrect username or password");
}


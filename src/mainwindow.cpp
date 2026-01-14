
// Changes made:
// 1) Add Account: removed duplicate toast + removed QMessageBox popup (toast only)
// 2) Withdraw toast text fixed ("Withdrew", arrow direction)
// 3) Small consistency: show toast error for simple “select an account” type issues (optional but included)
// 4) Kept QMessageBox for real failures (invalid session, not found, file open fail, exceptions)

#include "mainwindow.h"
#include "LoginWindow.hpp"
#include "ui_mainwindow.h"
#include "Toast.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QTextDocument>
#include <QPrinter>
#include <QAction>
#include <QWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QDebug>
#include <QTimer>
#include <QShowEvent>

#include "Customers.hpp"
#include "DebitCard.hpp"
#include "CreditCard.hpp"
#include "appFlow.h"


// -------------------- CTOR / DTOR --------------------

MainWindow::MainWindow(QWidget *parent, Services *services, std::shared_ptr<Customers> customer)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , services_(services)
    , currentCustomer_(std::move(customer))
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icons/resources/icons/Bank of World.ico"));

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);



    ui->tableAccounts->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableAccounts->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->tableAccounts->setSelectionMode(QAbstractItemView::SingleSelection);


    setWindowTitle("Customer Dashboard");

    ui->tableAccounts->verticalHeader()->setVisible(false);
    ui->tableAccounts->setShowGrid(false);
    ui->tableAccounts->setAlternatingRowColors(true);

    ui->tableAccounts->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableAccounts->horizontalHeader()->setStretchLastSection(true);
    ui->tableAccounts->setAlternatingRowColors(true);


    // ---- Cards table setup ----
    ui->tableCards->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableCards->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->tableCards->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableCards->verticalHeader()->setVisible(false);
    ui->tableCards->setShowGrid(false);
    ui->tableCards->setAlternatingRowColors(true);
    ui->tableCards->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);




    // -------------------- STYLESHEET --------------------
    this->setStyleSheet(R"(
    QMainWindow {
        background-color: #f0f3f7;
        font-family: 'Segoe UI', sans-serif;
        font-size: 11pt;
    }

    QToolBar {
        background-color: #ecf0f1;
        border-bottom: 1px solid #dcdcdc;
    }

    QToolButton {
        color: #1F497D;
        font-weight: bold;
    }
    QToolButton:hover {
        color: #16345c;
    }

    QPushButton {
        background-color: #1F497D;
        color: white;
        border-radius: 6px;
        padding: 6px 12px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: #16345c;
    }

    QTableWidget {
        background-color: #ffffff;
        color: #000000;
        border: 1px solid #dcdcdc;
        gridline-color: #dcdcdc;
        selection-background-color: #3498db;
        selection-color: white;
    }

    QHeaderView::section {
        background-color: #2980b9;
        color: white;
        padding: 4px;
        font-weight: bold;
    }

    QTableWidget { background-color: #ffffff; }
    QTableWidget QTableCornerButton::section { background-color: #2980b9; }

    QListWidget {
        background-color: #ffffff;
        border: 1px solid #dcdcdc;
        padding: 4px;
        color: #000000;
    }

    QListWidget::item:selected {
        background-color: #ecf0f1;
        color: #1F497D;
    }

    QLabel {
        background: transparent;
        color: #000000;
        font-weight: bold;
        padding: 2px;
    }

    QLineEdit, QComboBox, QTextEdit, QPlainTextEdit {
        background-color: #ffffff;
        color: #000000;
        border: 1px solid #dcdcdc;
        border-radius: 4px;
        padding: 2px 6px;
    }

    QLineEdit:disabled, QComboBox:disabled {
        background-color: #f5f5f5;
        color: #555555;
    }

    QScrollBar:vertical {
        width: 10px;
        background: #f0f3f7;
    }
    QScrollBar::handle:vertical {
        background: #2980b9;
        border-radius: 4px;
    }
    QScrollBar::add-line, QScrollBar::sub-line {
        height: 0px;
    }

    QMessageBox {
        background-color: #ffffff;
        color: #000000;
    }
    QMessageBox QPushButton {
        background-color: #1F497D;
        color: #ffffff;
        border-radius: 5px;
        padding: 4px 10px;
    }
    QMessageBox QPushButton:hover {
        background-color: #16345c;
    }

    QDialog {
        background-color: #ffffff;
        color: #000000;
        border: 1px solid #dcdcdc;
        border-radius: 8px;
    }

    QDialog QPushButton {
        background-color: #1F497D;
        color: white;
        border-radius: 6px;
        padding: 6px 12px;
        font-weight: bold;
    }
    QDialog QPushButton:hover {
        background-color: #16345c;
    }
)");

    // -------------------- TOOLBAR LOGIC --------------------
    QWidget* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->toolBar->addWidget(spacer);

    QAction* logoutAction = new QAction("Logout", this);
    connect(logoutAction, &QAction::triggered, this, &MainWindow::handleLogout);
    ui->toolBar->addAction(logoutAction);
    ui->toolBar->setStyleSheet("QToolButton { color: #1F497D; font-weight: bold; }");

    // -------------------- CUSTOMER INFO --------------------
    if (currentCustomer_) {
        statusBar()->showMessage("Logged in as: " + QString::fromStdString(currentCustomer_->getName()));
        displayCustomerInfo();
        refreshAccountList();
        refreshCardTable();
    } else {
        statusBar()->showMessage("Customer session invalid.");
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

// -------------------- ACCOUNTS --------------------

void MainWindow::on_addAccountButton_clicked()
{
    try {
        if (!currentCustomer_) {
            QMessageBox::warning(this, "Add Account", "Customer session invalid.");
            return;
        }

        QStringList types = {"Saving", "Checking"};
        bool ok = false;

        QString choice = QInputDialog::getItem(
            this, "Select Account Type", "Account Type:", types, 0, false, &ok);
        if (!ok || choice.isEmpty()) return;

        double initialBalance = QInputDialog::getDouble(
            this, "Initial Deposit", "Enter initial balance:", 0.0, 0, 1000000, 2, &ok);
        if (!ok || initialBalance <= 0) return;

        std::shared_ptr<BankAccount> newAccount;

        if (choice == "Saving") {
            newAccount = std::make_shared<SavingAccount>(initialBalance, 0.02);
        } else {
            newAccount = std::make_shared<CheckingAccount>(500.0, 50.0, 1000.0, 2000.0, initialBalance);
        }

        currentCustomer_->addAccount(newAccount);

        refreshAccountList();
        showToastSuccess(QString("%1 account created").arg(choice));
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, "Add Account failed", e.what());
        showToastError("Failed to create account");
    }
    catch (...) {
        QMessageBox::critical(this, "Add Account failed", "Unknown error occurred.");
        showToastError("Failed to create account");
    }
}

void MainWindow::on_closeAccountButton_clicked()
{
    if (!currentCustomer_) {
        QMessageBox::warning(this, "Close Account", "No customer selected!");
        return;
    }

    int row = ui->tableAccounts->currentRow();
    if (row < 0) {
        showToastError("Select an account first");
        return;
    }

    int accNumber = ui->tableAccounts->item(row, 0)->text().toInt();

    QMessageBox::StandardButton reply =
        QMessageBox::question(this, "Confirm Account Closure",
                              QString("Are you sure you want to close account #%1? This action cannot be undone.")
                                  .arg(accNumber),
                              QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    currentCustomer_->removeAccount(accNumber);
    refreshAccountList();
    showToastSuccess(QString("Closed account #%1").arg(accNumber));
}

// --- refreshAccountList() ---

void MainWindow::refreshAccountList()
{
    if (!ui || !ui->tableAccounts || !currentCustomer_) return;

    ui->tableAccounts->blockSignals(true);
    ui->tableAccounts->setUpdatesEnabled(false);

    long long selectedAccNum = -1;
    int prevRow = ui->tableAccounts->currentRow();
    if (prevRow >= 0) {
        if (auto *it = ui->tableAccounts->item(prevRow, 0)) {
            bool ok = false;
            selectedAccNum = it->text().toLongLong(&ok);
            (void)ok;
        }
    }

    auto accounts = currentCustomer_->getAccount();
    const int n = static_cast<int>(accounts.size());

    ui->tableAccounts->setRowCount(0);
    ui->tableAccounts->setRowCount(n);

    QStringList headers = {"Account Number", "Type", "Balance", "Customer ID", "Monthly Interests", "Daily Limits"};
    ui->tableAccounts->setColumnCount(headers.size());
    ui->tableAccounts->setHorizontalHeaderLabels(headers);
    ui->tableAccounts->setColumnHidden(3, true);

    for (int row = 0; row < n; ++row) {
        auto account = accounts[row];
        if (!account) continue;

        const bool isSaving = (std::dynamic_pointer_cast<SavingAccount>(account) != nullptr);
        const QString type = isSaving ? "Saving" : "Checking";

        ui->tableAccounts->setItem(row, 0, new QTableWidgetItem(QString::number(account->getAccountNumber())));
        ui->tableAccounts->setItem(row, 1, new QTableWidgetItem(type));
        ui->tableAccounts->setItem(row, 2, new QTableWidgetItem(QString::number(account->getAccountBalance(), 'f', 2)));

        double monthlyInterest = 0.0;
        if (auto savings = std::dynamic_pointer_cast<SavingAccount>(account)) {
            monthlyInterest = savings->getAccountBalance() * savings->getInterestRate() / 12.0;
        }
        ui->tableAccounts->setItem(row, 4, new QTableWidgetItem(QString::number(monthlyInterest, 'f', 2)));

        double dailyLimit = 0.0;
        if (auto checking = std::dynamic_pointer_cast<CheckingAccount>(account)) {
            dailyLimit = checking->getDailyWithdrawalLimit();
        }
        ui->tableAccounts->setItem(row, 5, new QTableWidgetItem(QString::number(dailyLimit, 'f', 2)));
    }


    if (selectedAccNum != -1) {
        for (int r = 0; r < ui->tableAccounts->rowCount(); ++r) {
            if (ui->tableAccounts->item(r, 0)->text().toLongLong() == selectedAccNum) {
                ui->tableAccounts->setCurrentCell(r, 0);
                break;
            }
        }
    }

    ui->tableAccounts->setUpdatesEnabled(true);
    ui->tableAccounts->blockSignals(false);

    ui->tableAccounts->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->tableAccounts->horizontalHeader()->setStretchLastSection(true);
}

// -------------------- TRANSACTIONS --------------------

void MainWindow::on_depositButton_clicked()
{
    if (!currentCustomer_) {
        QMessageBox::warning(this, "Deposit failed", "Customer session invalid.");
        return;
    }

    int row = ui->tableAccounts->currentRow();
    if (row < 0) {
        showToastError("Select an account first");
        return;
    }

    auto *item = ui->tableAccounts->item(row, 0);
    if (!item) {
        showToastError("Invalid selection");
        return;
    }
    int accNumber = item->text().toInt();

    std::shared_ptr<BankAccount> targetAccount = nullptr;
    for (auto& account : currentCustomer_->getAccount()) {
        if (account && account->getAccountNumber() == accNumber) {
            targetAccount = account;
            break;
        }
    }

    if (!targetAccount) {
        QMessageBox::critical(this, "Deposit failed", "Account not found in system registry.");
        return;
    }

    bool ok = false;
    double amount = QInputDialog::getDouble(
        this,
        "Make a Deposit",
        QString("Enter amount to deposit into account #%1:").arg(accNumber),
        1.00, 0.01, 1000000.00, 2, &ok);

    if (!ok || amount <= 0) return;

    if (targetAccount->applyDeposit(amount)) {
        refreshAccountList();
        showToastSuccess(QString("Deposited $%1 → #%2")
                             .arg(amount, 0, 'f', 2)
                             .arg(accNumber));
    } else {
        QMessageBox::critical(this, "Deposit Failed", "The transaction could not be processed.");
        showToastError("Deposit failed");
    }
}

void MainWindow::on_withdrawButton_clicked()
{
    if (!currentCustomer_) {
        QMessageBox::warning(this, "Withdrawal Failed", "Customer session invalid.");
        return;
    }

    int row = ui->tableAccounts->currentRow();
    if (row < 0) {
        showToastError("Select an account first");
        return;
    }

    auto *item = ui->tableAccounts->item(row, 0);
    if (!item) {
        showToastError("Invalid selection");
        return;
    }
    int accNumber = item->text().toInt();

    std::shared_ptr<BankAccount> targetAccount = nullptr;
    for (auto& account : currentCustomer_->getAccount()) {
        if (account && account->getAccountNumber() == accNumber) {
            targetAccount = account;
            break;
        }
    }

    if (!targetAccount) {
        QMessageBox::critical(this, "Withdrawal Failed", "Account not found.");
        return;
    }

    bool ok = false;
    double amount = QInputDialog::getDouble(
        this,
        "Make a Withdrawal",
        QString("Enter amount to withdraw from account #%1:").arg(accNumber),
        1.00, 0.01, 1000000.00, 2, &ok);

    if (!ok || amount <= 0.0) return;

    try {
        if (targetAccount->applyWithdraw(amount)) {
            refreshAccountList();
            showToastSuccess(QString("Withdrew $%1 ← #%2")
                                 .arg(amount, 0, 'f', 2)
                                 .arg(accNumber));
        } else {
            QMessageBox::warning(this, "Withdrawal Failed", "Withdrawal was not completed.");
            showToastError("Withdrawal failed");
        }
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, "Withdrawal Failed", QString("Error: %1").arg(e.what()));
        showToastError("Withdrawal error");
    }
}

void MainWindow::on_showTransactionsButton_clicked()
{
    if (!currentCustomer_) {
        QMessageBox::warning(this, "Error", "Customer session invalid.");
        return;
    }

    int row = ui->tableAccounts->currentRow();
    if (row < 0) {
        showToastError("Select an account first");
        return;
    }

    int accNumber = ui->tableAccounts->item(row, 0)->text().toInt();

    std::shared_ptr<BankAccount> targetAccount = nullptr;
    for (auto& account : currentCustomer_->getAccount()) {
        if (account && account->getAccountNumber() == accNumber) {
            targetAccount = account;
            break;
        }
    }

    if (!targetAccount) {
        QMessageBox::critical(this, "History Error", "Account not found in system registry.");
        return;
    }

    refreshTransactionList(targetAccount);
}

void MainWindow::refreshTransactionList(std::shared_ptr<BankAccount> account)
{
    if (!ui->transactionList) return;

    ui->transactionList->clear();

    const auto& transactions = account->getTransactions();

    if (transactions.empty()) {
        ui->transactionList->addItem("No transactions found for this account.");
        return;
    }

    for (const auto& t : transactions) {
        ui->transactionList->addItem(QString::fromStdString(t.toString()));
    }
}

// -------------------- CARDS --------------------

void MainWindow::on_replaceCardButton_clicked()
{
    if (!currentCustomer_) return;

    const auto& cards = currentCustomer_->getAllCards(); // your API
    if (cards.empty()) {
        showToastError("No cards found to replace");
        return;
    }

    auto oldCard = cards[0];

    QMessageBox::StandardButton reply =
        QMessageBox::question(this, "Replace Card",
                              QString("Are you sure you want to replace card ending in %1?")
                                  .arg(QString::fromStdString(oldCard->getCardNumber().substr(12))),
                              QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) return;

    std::shared_ptr<Card> newCard = services_->replaceCard(oldCard);

    if (newCard) {
        showToastSuccess("Card replaced and activated");
        refreshCardTable();
    } else {
        QMessageBox::critical(this, "Error", "Failed to generate replacement card.");
        showToastError("Replace failed");
    }
}

// =============================================== ui design changed ===============================================
// void MainWindow::refreshCardList()
// {
//     if (!ui->cardListWidget) return;

//     ui->cardListWidget->clear();
//     if (!currentCustomer_) return;

//     for (const auto& accountPtr : currentCustomer_->getAccount()) {
//         auto checkingAccount = std::dynamic_pointer_cast<CheckingAccount>(accountPtr);
//         if (!checkingAccount) continue;

//         for (const auto& debitCard : checkingAccount->getDebitCards()) {
//             QString displayText =
//                 QString("Debit Card: %1 | Exp: %2 | CVV: %3 | Activated: %4 | Daily Withdrawal Limit: %5 | Contactless Enable: %6")
//                     .arg(QString::fromStdString(debitCard->getCardNumber()))
//                     .arg(QString::fromStdString(debitCard->getDisplayExpiration()))
//                     .arg(QString::fromStdString(debitCard->getMaskedCvv()))
//                     .arg(debitCard->isActivated() ? "Yes" : "No")
//                     .arg(debitCard->getDailyWithdrawalLimit(), 0, 'f', 2)
//                     .arg(debitCard->isContactlessEnabled() ? "Yes" : "No");
//             ui->cardListWidget->addItem(displayText);
//         }

//         for (const auto& creditCard : checkingAccount->getCreditCards()) {
//             QString displayText =
//                 QString("Credit Card: %1 | Exp: %2 | CVV: %3 | Limit: $%4 | Balance: $%5 | Activated: %6 | Contactless Enable: %7")
//                     .arg(QString::fromStdString(creditCard->getCardNumber()))
//                     .arg(QString::fromStdString(creditCard->getDisplayExpiration()))
//                     .arg(QString::fromStdString(creditCard->getMaskedCvv()))
//                     .arg(creditCard->getCreditLimit(), 0, 'f', 2)
//                     .arg(creditCard->getCurrentBalance(), 0, 'f', 2)
//                     .arg(creditCard->isActivated() ? "Yes" : "No")
//                     .arg(creditCard->isContactlessEnabled() ? "Yes" : "No");
//             ui->cardListWidget->addItem(displayText);
//         }
//     }
// }

// -------------------- EXPORT --------------------

void MainWindow::on_exportTransactionsButton_clicked()
{
    if (!currentCustomer_) {
        QMessageBox::warning(this, "Export Failed", "Customer session invalid.");
        return;
    }

    int row = ui->tableAccounts->currentRow();
    if (row < 0) {
        showToastError("Select an account first");
        return;
    }

    int accNumber = ui->tableAccounts->item(row, 0)->text().toInt();

    std::shared_ptr<BankAccount> targetAccount = nullptr;
    for (auto& account : currentCustomer_->getAccount()) {
        if (account && account->getAccountNumber() == accNumber) {
            targetAccount = account;
            break;
        }
    }

    if (!targetAccount) {
        QMessageBox::critical(this, "Export Failed", "Account not found.");
        return;
    }

    QStringList formats = {"CSV", "PDF"};
    bool ok = false;
    QString format = QInputDialog::getItem(this, "Export Format", "Choose file type:", formats, 0, false, &ok);
    if (!ok || format.isEmpty()) return;

    if (format == "CSV") {
        QString fileName = QFileDialog::getSaveFileName(this,
                                                        "Save Transactions As CSV",
                                                        QString("transactions_%1.csv").arg(accNumber),
                                                        "CSV Files (*.csv)");
        if (fileName.isEmpty()) return;

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, "Export Failed", "Cannot open file for writing.");
            return;
        }

        QTextStream out(&file);
        out << "Date,Type,Amount,Balance After,Description,Timestamp\n";
        for (const auto& t : targetAccount->getTransactions()) {
            out << QString::fromStdString(t.toCSV()) << "\n";
        }
        file.close();

        showToastSuccess("Exported CSV");
        return;
    }

    // PDF
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Save Transactions As PDF",
                                                    QString("transactions_%1.pdf").arg(accNumber),
                                                    "PDF Files (*.pdf)");
    if (fileName.isEmpty()) return;

    QString html;
    html += "<h2>Transaction History for Account #" + QString::number(accNumber) + "</h2>";
    html += "<p>Customer: " + QString::fromStdString(currentCustomer_->getName()) + "</p>";
    html += "<table border='1' cellspacing='0' cellpadding='4' width='100%'>";
    html += "<tr style='background-color:#f0f0f0; font-weight:bold;'>";
    html += "<th>Date</th><th>Type</th><th>Amount</th><th>Balance After</th><th>Description</th><th>Account Number</th>";
    html += "</tr>";

    const auto& transactions = targetAccount->getTransactions();
    if (transactions.empty()) {
        html += "<tr><td colspan='6' align='center'>No transactions found.</td></tr>";
    } else {
        for (const auto& t : transactions) {
            QStringList fields = QString::fromStdString(t.toCSV()).split(',');
            html += "<tr>";
            for (int i = 0; i < fields.size(); ++i) {
                QString field = fields[i].trimmed();
                if (i == 2 || i == 3) {
                    html += "<td align='right'>$" + field + "</td>";
                } else {
                    html += "<td>" + field + "</td>";
                }
            }
            html += "</tr>";
        }
    }
    html += "</table>";

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&printer);

    showToastSuccess("Exported PDF");
}

// -------------------- CUSTOMER INFO --------------------

void MainWindow::displayCustomerInfo()
{
    if (!currentCustomer_) return;

    ui->nameLabel->setText(QString("Name: %1").arg(QString::fromStdString(currentCustomer_->getName())));
    ui->emailLabel->setText(QString("Email: %1").arg(QString::fromStdString(currentCustomer_->getEmail())));
    ui->ageLabel->setText(QString("Age: %1").arg(currentCustomer_->getAge()));
    ui->addressLabel->setText(QString("Address: %1").arg(QString::fromStdString(currentCustomer_->getAddress())));
}

// -------------------- LOGOUT --------------------

void MainWindow::handleLogout()
{
    this->close();
    showLoginAndRoute(services_);
}

// -------------------- TOAST HELPERS --------------------

void MainWindow::showToast(const QString& msg, int ms)
{
    auto *t = new Toast(msg, Toast::Info);
    t->popupCentered(this, ms);
}

void MainWindow::showToastSuccess(const QString& msg, int ms)
{
    auto *t = new Toast(msg, Toast::Success);
    t->popupCentered(this, ms);
}

void MainWindow::showToastError(const QString& msg, int ms)
{
    auto *t = new Toast(msg, Toast::Error);
    t->popupCentered(this, ms);
}

void MainWindow::refreshCardTable()
{
    if (!ui->tableCards || !currentCustomer_) return;

    const auto& cards = currentCustomer_->getAllCards();

    ui->tableCards->clearContents();
    ui->tableCards->setRowCount((int)cards.size());
    ui->tableCards->setColumnCount(6);
    ui->tableCards->setHorizontalHeaderLabels(
        {"Type", "Last 4", "Activated", "Expired", "Contactless", "Limit/Util"}
        );

    for (int r = 0; r < (int)cards.size(); ++r) {
        auto c = cards[r];
        if (!c) continue;

        const std::string num = c->getCardNumber();
        QString last4 = (num.size() >= 4)
                            ? QString::fromStdString(num.substr(num.size() - 4))
                            : QString::fromStdString(num);

        QString type = "Card";
        QString contactless = "N/A";
        QString limitOrUtil = "N/A";

        if (std::dynamic_pointer_cast<DebitCard>(c)) type = "Debit";
        else if (std::dynamic_pointer_cast<CreditCard>(c)) type = "Credit";

        // Contactless + limit/utilization
        if (auto d = std::dynamic_pointer_cast<DebitCard>(c)) {
            contactless = d->isContactlessEnabled() ? "Yes" : "No";
            limitOrUtil = QString("$%1").arg(d->getDailyWithdrawalLimit(), 0, 'f', 0);
        }
        else if (auto cr = std::dynamic_pointer_cast<CreditCard>(c)) {
            contactless = cr->isContactlessEnabled() ? "Yes" : "No";
            limitOrUtil = QString("%1%").arg(cr->getUtilizationPercentage(), 0, 'f', 1);
        }

        auto *typeItem = new QTableWidgetItem(type);
        // store full card number in UserRole so we can find it later
        typeItem->setData(Qt::UserRole, QString::fromStdString(num));

        ui->tableCards->setItem(r, 0, typeItem);
        ui->tableCards->setItem(r, 1, new QTableWidgetItem(last4));
        ui->tableCards->setItem(r, 2, new QTableWidgetItem(c->isActivated() ? "Yes" : "No"));
        ui->tableCards->setItem(r, 3, new QTableWidgetItem(c->isExpired() ? "Yes" : "No"));
        ui->tableCards->setItem(r, 4, new QTableWidgetItem(contactless));
        ui->tableCards->setItem(r, 5, new QTableWidgetItem(limitOrUtil));
    }
}

// helper function
std::shared_ptr<Card> MainWindow::getSelectedCard() const
{
    int row = ui->tableCards->currentRow();
    if (row < 0) return nullptr;

    auto *item = ui->tableCards->item(row, 0);
    if (!item) return nullptr;

    QString cardNumber = item->data(Qt::UserRole).toString();
    if (cardNumber.isEmpty()) return nullptr;

    for (auto &c : currentCustomer_->getAllCards()) {
        if (c && QString::fromStdString(c->getCardNumber()) == cardNumber)
            return c;
    }
    return nullptr;
}

void MainWindow::on_activateCardButton_clicked()
{
    auto card = getSelectedCard();
    if (!card) { showToastError("Select a card first"); return; }
    card->setActivated(true);
    refreshCardTable();
    showToastSuccess("Card activated");
}

void MainWindow::on_deactivateCardButton_clicked()
{
    auto card = getSelectedCard();
    if (!card) { showToastError("Select a card first"); return; }
    card->setActivated(false);
    refreshCardTable();
    showToastSuccess("Card deactivated");
}

void MainWindow::on_markExpiredButton_clicked()
{
    auto card = getSelectedCard();
    if (!card) { showToastError("Select a card first"); return; }
    card->markExpired();
    card->setActivated(false);
    refreshCardTable();
    showToastSuccess("Card expired");
}

void MainWindow::on_toggleContactlessButton_clicked()
{
    auto card = getSelectedCard();
    if (!card) { showToastError("Select a card first"); return; }

    if (auto d = std::dynamic_pointer_cast<DebitCard>(card)) {
        d->enableContactless(!d->isContactlessEnabled());
    } else if (auto c = std::dynamic_pointer_cast<CreditCard>(card)) {
        c->enableContactless(!c->isContactlessEnabled());
    } else {
        showToastError("Unknown card type");
        return;
    }

    refreshCardTable();
    showToastSuccess("Contactless updated");
}

void MainWindow::on_setDailyLimitButton_clicked()
{
    auto card = getSelectedCard();
    auto d = std::dynamic_pointer_cast<DebitCard>(card);
    if (!d) { showToastError("Select a debit card"); return; }

    bool ok = false;
    double newLimit = QInputDialog::getDouble(
        this, "Daily Withdrawal Limit",
        "Enter new daily limit:",
        d->getDailyWithdrawalLimit(),
        0, 1000000, 2, &ok
        );
    if (!ok) return;

    d->SetDailyWithdrawalLimit(newLimit);
    refreshCardTable();
    showToastSuccess("Daily limit updated");
}

void MainWindow::showEvent(QShowEvent* e)
{
    QMainWindow::showEvent(e);

    if (!firstShow_) return;
    firstShow_ = false;

    QTimer::singleShot(0, this, [this] {
        setWindowState(windowState() | Qt::WindowMaximized);
        raise();
        activateWindow();
    });
}

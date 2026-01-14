#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "Services.hpp"
#include "Customers.hpp"
#include "BankAccount.hpp" // Include for use in method signatures if needed

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Parameterized constructor to receive Services and the logged-in Customer
    explicit MainWindow(QWidget *parent = nullptr,
                        Services* services = nullptr,
                        std::shared_ptr<Customers> customer = nullptr);
    ~MainWindow();

private slots:
    void on_addAccountButton_clicked();
    void on_closeAccountButton_clicked();
    void on_depositButton_clicked();
    void on_withdrawButton_clicked();
    void on_showTransactionsButton_clicked();
    void on_replaceCardButton_clicked();
    void on_exportTransactionsButton_clicked();

    void on_activateCardButton_clicked();
    void on_deactivateCardButton_clicked();
    void on_toggleContactlessButton_clicked();
    void on_setDailyLimitButton_clicked();
    void on_markExpiredButton_clicked();




    
    void handleLogout();

protected:
    void showEvent(QShowEvent* e) override;


private:
    // Customer Dashboard utility methods
    void refreshAccountList();
    void refreshTransactionList(std::shared_ptr<BankAccount> account);
    void displayCustomerInfo();
    void refreshCardList();
    bool firstShow_ = true;


    void refreshCardTable();
    std::shared_ptr<Card> getSelectedCard() const;



    // Member variables
    Ui::MainWindow *ui;
    Services* services_;
    std::shared_ptr<Customers> currentCustomer_;


    // =======================
    void showToast(const QString& msg, int ms = 1800);
    void showToastSuccess(const QString& msg, int ms = 1800);
    void showToastError(const QString& msg, int ms = 2200);

};

#endif // MAINWINDOW_H

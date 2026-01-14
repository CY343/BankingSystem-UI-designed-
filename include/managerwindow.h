#ifndef MANAGERWINDOW_H
#define MANAGERWINDOW_H

#include <QMainWindow>
#include<memory>
#include<vector>
#include"Services.hpp"
#include"Customers.hpp"
#include"BankAccount.hpp"

namespace Ui {
class ManagerWindow;
}

class ManagerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ManagerWindow(QWidget *parent = nullptr, Services* services = nullptr);
    ~ManagerWindow();

private slots:
    // slots for managing customers
    void on_addCustomerButton_clicked();
    void on_deleteCustomerButton_clicked();
    void handleLogout();

    // slot to handle seleting a customer to view their accounts
    void on_customerTableWidget_cellClicked(int row, int column);
private:
    Ui::ManagerWindow *ui;
    Services* services_;

    // core display function
    void refreshCustomerList();
    void displayCustomerAccounts(const std::string& customerId);

     bool firstShow_ = true;



    // list to hold the currently fetched customers for look lookup
    std::vector<std::shared_ptr<Customers>> currentCustomerList_;

protected:
    void showEvent(QShowEvent* e) override;
};

#endif // MANAGERWINDOW_H

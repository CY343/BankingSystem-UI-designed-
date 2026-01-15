#include"seedData.hpp"
#include "Customers.hpp"
#include "SavingAccount.hpp"
#include "CheckingAccount.hpp"
#include <QDebug>

void seedTestData(Services& services)
{
    // ----------- Add test customers and accounts -----------
    std::shared_ptr<Customers> customer1 = std::make_shared<Customers>(
        "Yue Wayne", "bruce@wayne.com", "123-456-7890", 35, "123-59 34th ave"
        );

    std::shared_ptr<Customers> customer2 = std::make_shared<Customers>(
        "Deby", "deby@dailyplanet.com", "987-654-3210", 30, "321-23 55th ave"
        );

    // Create accounts with sufficient balances
    auto saving1 = std::make_shared<SavingAccount>(1000.0, 0.02);
    auto checking1 = std::make_shared<CheckingAccount>(
        1000.0, 50.0, 1500.0, 500.0, 6000.0  // initial balance = 6000
        );

    customer1->addAccount(saving1);
    customer1->addAccount(checking1);

    try {
        // Issue a Debit Card
        checking1->issueDebitCard();
        for (auto& card : checking1->getDebitCards()) {
            card->enableContactless(true); // enable contactless
            card->SetDailyWithdrawalLimit(500.0); // daily limit
            card->SetDailySpendAmount(3000.00); //set daily spending amount
        }

        // Add a Credit Card with limit safely below 2x balance
        double creditLimit = 5000.0; // < 2 * 6000, safe
        checking1->addCreditCard(creditLimit, true, checking1);

        qInfo() << "Credit Card successfully issued!";

        // Display Debit Card info
        const auto& debitCards = checking1->getDebitCards();

        for (auto& card : checking1->getCreditCards()) {

            card->enableContactless(true);

        }


        for (const auto& card : debitCards) {
             qDebug() << "Debit Card: "<< QString::fromStdString(card->getCardNumber())
                      << " | Exp: " << QString::fromStdString(card->getDisplayExpiration())
                      << " | CVV: " << QString::fromStdString(card->getMaskedCvv())
                      << " | Activated: " << (card->isActivated() ? "Yes" : "No")
                      << " | Contactless: " << (card->isContactlessEnabled()? "Yes": "No");

        }

        // Display Credit Card info
        const auto& creditCards = checking1->getCreditCards();
        for (const auto& card : creditCards) {
            qDebug() << "Credit Card: " << QString::fromStdString(card->getCardNumber())
                     << " | Exp: " << QString::fromStdString(card->getDisplayExpiration())
                     << " | CVV: " << QString::fromStdString(card->getMaskedCvv())
                     << " | Activated: " << (card->isActivated() ? "Yes" : "No")
                     << " | Contactless: " << (card->isContactlessEnabled()? "Yes": "No");

        }

    } catch(const std::exception& e) {
        qWarning() << "[SeedData] Failed to initialize cards:" << e.what();
    }

    auto saving2 = std::make_shared<SavingAccount>(2000.0, 0.03);
    customer2->addAccount(saving2);

    services.addCustomers({customer1, customer2});
}

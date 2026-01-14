#include <QApplication>
#include <QIcon>
#include <iostream>
#include <stdexcept>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include "LoginWindow.hpp"
#include "mainwindow.h"
#include "managerwindow.h"
#include "Services.hpp"
#include "seedData.hpp"
#include "appFlow.h"
#include <QTimer>

void showLoginAndRoute(Services* services)
{
    LoginWindow login(nullptr, services);

    if (login.exec() != QDialog::Accepted) {
        return; // user closed login
    }

    auto loggedInCustomer = login.getLoggedInCustomer();

    if (loggedInCustomer) {
        auto *w = new MainWindow(nullptr, services, loggedInCustomer);
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->show();
    } else {
        auto *w = new ManagerWindow(nullptr, services);
        w->setAttribute(Qt::WA_DeleteOnClose);
        w->show();

    }
}



static void myTerminateHandler()
{
    qDebug() << "std::terminate called! Unhandled exception or fatal error.";
    std::abort();
}

static const char* kConnName = "banking_conn";

static bool openDb()
{
    qDebug() << "Available SQL drivers:" << QSqlDatabase::drivers();

    if (QSqlDatabase::contains(kConnName)) {
        QSqlDatabase::removeDatabase(kConnName);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QODBC", kConnName);

    db.setDatabaseName(
        "Driver={MySQL ODBC 9.5 Unicode Driver};"
        "Server=127.0.0.1;"
        "Port=3306;"
        "Database=BankingSystem;"
        "User=bankapp;"
        "Password=bankapp1234;"
        "Option=3;"
        "NO_SSPS=1;"
        );

    if (!db.open()) {
        qDebug() << "DB open failed:" << db.lastError().text();
        return false;
    }

    qDebug() << "DB connected via ODBC!";
    return true;
}

static void closeDb()
{
    {
        QSqlDatabase db = QSqlDatabase::database(kConnName);
        if (db.isValid()) db.close();
    }
    QSqlDatabase::removeDatabase(kConnName);
}

int main(int argc, char *argv[])
{
    std::set_terminate(myTerminateHandler);

    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/resources/icons/Bank of World.ico"));
    QApplication::setQuitOnLastWindowClosed(false);


    Services services;
    seedTestData(services);

    if (!openDb()) return 1;

    {
        QSqlDatabase db = QSqlDatabase::database(kConnName);
        QSqlQuery q(db);
        if (q.exec("SELECT DATABASE() AS db, USER() AS u, @@hostname AS host, @@port AS p")) {
            q.next();
            qDebug() << "Qt connected to:"
                     << q.value("db").toString()
                     << q.value("u").toString()
                     << q.value("host").toString()
                     << q.value("p").toInt();
        } else {
            qDebug() << "DB identity query failed:" << q.lastError().text();
        }
    }

    {
        QSqlDatabase db = QSqlDatabase::database(kConnName);
        QSqlQuery q(db);
        if (q.exec("SELECT 1")) qDebug() << "DB ping OK";
        else qDebug() << "DB ping failed:" << q.lastError().text();
    }

    {
        QSqlDatabase db = QSqlDatabase::database(kConnName);
        QSqlQuery q(db);

        if (!db.transaction()) {
            qDebug() << "Failed to start transaction:" << db.lastError().text();
        }

        QString accId = QUuid::createUuid().toString(QUuid::WithoutBraces);

        q.prepare(R"(
            INSERT INTO accounts (id, customer_id, type, balance, is_active)
            VALUES (:id, :cid, :type, :bal, :active)
        )");

        q.bindValue(":id", accId);
        q.bindValue(":cid", "demo_customer_1");
        q.bindValue(":type", "CHECKING");
        q.bindValue(":bal", 500.00);
        q.bindValue(":active", 1);

        if (!q.exec()) {
            qDebug() << "Insert failed:" << q.lastError().text();
            qDebug() << "Native:" << q.lastError().nativeErrorCode();
            qDebug() << "DB text:" << q.lastError().databaseText();

            if (!db.rollback()) {
                qDebug() << "Rollback failed:" << db.lastError().text();
            } else {
                qDebug() << "Rolled back transaction.";
            }
        } else {
            if (!db.commit()) {
                qDebug() << "Commit failed:" << db.lastError().text();
            } else {
                qDebug() << "Insert OK + committed, id =" << accId;
            }
        }
    }
    // ================================================================================

    showLoginAndRoute(&services);

    // Run Qt event loop ONCE
    int rc = a.exec();

    closeDb();
    return rc;
}

#include "CheckingAccount.hpp"
#include <memory>
#include <stdexcept>
#include <iostream>   
#include <ctime>  


    /**
     * @brief Constructs a new CheckingAccount object with specified paraw CheckingAccount by setting up
     * overdraft limits, monthlymeters.
     *
     * This constructor initializes a ne fees, waiver balances, and withdrawal limits.
     * It also initializes base class BankAccount with zero values.
     *
     * @param overdraft_limit The maximum amount allowed to be overdrawn.
     * @param monthly_fee The monthly maintenance fee for the account.
     * @param fee_waiver_balance The minimum balance required to waive the monthly fee.
     * @param daily_Withdrawal_limit The maximum amount allowed for daily withdrawals.
     */
    CheckingAccount::CheckingAccount(double overdraft_limit,
                                    double monthly_fee,
                                    double fee_waiver_balance,
                                    double daily_Withdrawal_limit,
                                    double initial_balance)
                                    :
                                    BankAccount(0.0, 0.0, initial_balance, 0.0, "None"),
                                    overdraft_limit_(overdraft_limit),
                                    monthly_maintenance_fee_(monthly_fee),
                                    minimum_balance_waiver_(fee_waiver_balance),
                                    has_overdraft_protection_(false),
                                    monthly_transaction_count_(0),
                                    daily_withdrawal_limit_(daily_Withdrawal_limit),
                                    is_business_account_(false){}


    /**
     * @brief Constructs a CheckingAccount using an existing account identifier.
     *
     * This constructor is intended for restoring or reconstructing accounts
     * that were previously created and already have an assigned account number.
     * Typical use cases include loading accounts from persistent storage,
     * importing external data, or controlled testing scenarios.
     *
     * @param id Existing account number to assign to this account.
     * @param balance Initial account balance.
     * @param accountType Account type label (e.g., "CHECKING").
     *
     * @note This constructor does not generate a new account number.
     * @warning Callers are responsible for ensuring the provided account number
     *          is unique within the system.
     */
    CheckingAccount::CheckingAccount(int id, double balance, const std::string& accountType)
            :BankAccount(id, balance, accountType),  // Pass to base class
            overdraft_limit_(0.0),
            monthly_maintenance_fee_(0.0),
            minimum_balance_waiver_(1500.0),
            has_overdraft_protection_(false),
            is_business_account_(false) {}
    /**
     * @brief Sets the overdraft limit for the checking account.
     *
     * Updates the overdraft limit to the specified value if it is non-negative.
     *
     * @param limit The new overdraft limit to be set.
     * @return true If the limit is valid (non-negative) and was successfully set.
     * @return false If the provided limit is negative.
     */
    bool CheckingAccount::setOverdraftLimit(double limit)
    {
        if (limit < 0)
        {
            return false;
        }
        overdraft_limit_ = limit;
            return true;
    }

    /**
     * @brief Retrieves the current overdraft limit of the checking account.
     *
     * @return The current overdraft limit.
     */
    double CheckingAccount::getOverdraftLimit() const
    {
        return overdraft_limit_;
    }

    /**
     * @brief Attempts to withdraw money from the checking account.
     *
     * Enforces overdraft rules and (optionally) overdraft protection using a backup account.
     *
     * @param amount Amount to withdraw (must be > 0).
     * @return true if the withdrawal succeeds; false otherwise.
     *
     * @note This implementation does not throw; it returns false on failure.
     */
    bool CheckingAccount::applyWithdraw(const double& amount)
    {
        if (amount <= 0) {
            std::cerr << "[applyWithdraw] Invalid withdrawal amount: " << amount << '\n';
            return false;
        }

        const double currentBalance = getAccountBalance();
        const double available = currentBalance + overdraft_limit_;  // overdraft_limit_ is positive

        if (amount > available) {
            if (!has_overdraft_protection_) {
                std::cerr << "[applyWithdraw] No overdraft protection. Withdrawal denied.\n";
                return false;
            }

            auto backup = overdraft_backup_account_.lock();
            if (!backup) {
                std::cerr << "[applyWithdraw] Backup account pointer expired or invalid.\n";
                return false;
            }

            // Exact amount needed from backup to make this withdrawal possible
            const double neededFromBackup = amount - available;

            if (!backup->applyWithdraw(neededFromBackup)) {
                std::cerr << "[applyWithdraw] Backup account withdrawal failed.\n";
                return false;
            }

            // Main account ends at the overdraft floor
            setBalanceInternal(-overdraft_limit_);

            addTransaction(Transaction::WITHDRAWAL, amount, time(nullptr));
            monthly_transaction_count_++;
            setLowBalance();

            return true;
        }

        // Normal withdrawal within available balance
        setBalanceInternal(currentBalance - amount);

        addTransaction(Transaction::WITHDRAWAL, amount, time(nullptr));
        monthly_transaction_count_++;
        setLowBalance();

        return true;
    }


    /**
     * @brief Toggles the account type between business and personal.
     *
     * Flips the value of the is_business_account_ flag.
     * If it was a business account, it becomes a personal account, and vice versa.
     */
    void CheckingAccount::convertTOBusinessAccount()
    {
        is_business_account_ = !is_business_account_;
    }

    /**
     * @brief Retrieves the monthly maintenance fee for the checking account.
     *
     * @return The current monthly maintenance fee.
     */
    double CheckingAccount::getMonthlyfee() const
    {
        return monthly_maintenance_fee_;
    }

    /**
     * @brief Retrieves the daily withdrawal limit for the checking account.
     *
     * @return The current daily withdrawal limit.
     */
    double CheckingAccount::getDailyWithdrawalLimit() const
    {
        return daily_withdrawal_limit_;
    }

    /**
     * @brief Retrieves the number of transactions made in the current month.
     *
     * @return The current monthly transaction count.
     */
    int CheckingAccount::getMonthlyTransactionCount() const
    {
        return monthly_transaction_count_;
    }

    /**
     * @brief Applies the monthly maintenance fee to the checking account if applicable.
     *
     * If the account does not qualify for a fee waiver, the monthly maintenance fee
     * is withdrawn from the account and recorded as a fee transaction.
     * If the account qualifies for the waiver, the onFeeWaived() handler is called.
     */
    void CheckingAccount::applyMonthlyMaintenanceFee()
    {
        if(!qualifiesForFeeWaiver())
        {
            BankAccount::applyWithdraw(monthly_maintenance_fee_);
            addTransaction(Transaction::FEE, monthly_maintenance_fee_, time(nullptr));
        }
        else
        {
            onFeeWaived();
        }
    }

    /**
     * @brief Notifies that the monthly maintenance fee has been waived.
     *
     * Prints a message to standard output indicating that the fee was waived,
     * along with the account number and current balance.
     */
    void CheckingAccount::onFeeWaived() const
    {
        std::cout << "Fee waived for account #" << getAccountNumber()
                << " (Balance: $" << getAccountBalance() << ")\n";
    }


    /**
     * @brief Checks if the account qualifies for a monthly fee waiver.
     *
     * Determines whether the current account balance meets or exceeds
     * the minimum balance required to waive the monthly maintenance fee.
     *
     * @return true If the balance is sufficient to waive the fee.
     * @return false Otherwise.
     */
    bool CheckingAccount::qualifiesForFeeWaiver() const
    {
        return getAccountBalance() >= minimum_balance_waiver_;
    }

    /**
     * @brief Issues a new debit card associated with this account.
     *
     * @param card_number The card number.
     * @param expiration The expiration date in MM/YY format.
     * @param cvv The card's CVV code.
     */
    void CheckingAccount::issueDebitCard()
    {
    try{
            auto self = std::static_pointer_cast<CheckingAccount>(shared_from_this());
            const std::string initialPin = "0000"; // user will force to change its pin
            auto debitCard = std::make_shared<DebitCard>(
                self,
                daily_withdrawal_limit_,
                initialPin
            );

            debit_cards_.push_back(debitCard);
            std::cout << "Debit card issued for account #"
                    << getAccountNumber() << std::endl;

    }

    catch(const std::bad_weak_ptr& e){

            std::cerr << "Failed to issue debit card: Object not owed by shared_ptr."
                    << "Create accounts using make_shared. Error: " << e.what() << std::endl;
    }

    catch(const std::exception& e){

            std::cerr << "Failed to issue debit card " << e.what() << std::endl;
    }

    }

    /**
     * @brief Blocks a debit card associated with this checking account.
     *
     * Searches for the debit card with the specified card number and
     * deactivates it if found. Marks the card as expired.
     *
     * @param cardNumber The card number of the debit card to block.
     * @return true If the card was found and successfully blocked.
     * @return false If no matching card was found.
     */
    bool CheckingAccount::blockDebitCard(const std::string &cardNumber)
    {
        for(auto& card: debit_cards_)
        {
            if(card->getCardNumber() == cardNumber)
            {
                card->setActivated(false);
                card->markExpired();
                std::cout<< "Debit card " << cardNumber << "blocked for account #" << getAccountNumber() << std::endl;
                return true;
            }
        }
            std::cerr << "Debit card " << cardNumber << " not found for account " << getAccountNumber() << std::endl;
            return false;
    }

    /**
     * @brief Adds a new credit card linked to this checking account.
     *
     * Validates that the requested credit limit does not exceed twice the current
     * account balance. Throws std::invalid_argument if validation fails.
     *
     * If valid, creates a new CreditCard with the specified credit limit, activation
     * status, and linked bank account, then adds it to the list of credit cards.
     *
     * @param creditLimit The credit limit for the new credit card.
     * @param isActivated Whether the credit card is initially activated.
     * @param linked_account A shared pointer to the linked bank account.
     *
     * @throws std::invalid_argument If creditLimit exceeds twice the account balance.
     */
    void CheckingAccount::addCreditCard(double creditLimit, bool isActivated, std::shared_ptr<BankAccount> linked_account)
    {
        if(creditLimit > getAccountBalance() * 2)
        {
            throw std::invalid_argument("Credit limit exceeds 2 times account balance.");
        }

        // Auto-generate credit card with BIN prefix for Visa/Mastercard
        credit_cards_.emplace_back
        (
        std::make_shared<CreditCard>(
            creditLimit,
            isActivated,
            linked_account)
        );
    }


    /**
     * @brief Links a backup bank account for overdraft protection.
     *
     * Stores a weak reference to the provided backup account and enables
     * overdraft protection on this checking account.
     *
     * @param backup Shared pointer to the backup bank account used for overdraft protection.
     */
    void CheckingAccount::linkOverdraftProtection(std::shared_ptr<BankAccount> backup)
    {
        overdraft_backup_account_ = backup;
        has_overdraft_protection_ = true;
    }


    /**
     * @brief Checks if overdraft protection is enabled on this account.
     *
     * @return true If overdraft protection is currently enabled.
     * @return false Otherwise.
     */
    bool CheckingAccount::hasOverdraftProtection() const
    {
        return has_overdraft_protection_;
    }

    /**
     * @brief Retrieves a shared pointer to the overdraft backup account.
     *
     * Attempts to lock the weak pointer to the backup account and return
     * a shared pointer. If the backup account no longer exists, returns nullptr.
     *
     * @return std::shared_ptr<BankAccount> Shared pointer to the backup account, or nullptr if expired.
     */

    std::shared_ptr<BankAccount> CheckingAccount::getBackupAccount()const
    {
        return overdraft_backup_account_.lock();
    }

    void CheckingAccount::applyInterest()
    {
        if (getAccountBalance() > minimum_balance_waiver_) {
            // Waive monthly fee if balance is high enough
            monthly_maintenance_fee_ = 0;
        } else {
            // Apply monthly fee
            BankAccount::applyWithdraw(monthly_maintenance_fee_);
            addTransaction(Transaction::FEE, monthly_maintenance_fee_, time(nullptr));
        }
    }

    /**
     * @brief Checks if the account is a business account.
     *
     * @return true If the account is designated as a business account.
     * @return false Otherwise.
     */
    bool CheckingAccount::isBusinessAccount() const
    {
        return is_business_account_;
    }

    /**
     * @brief Sets the account balance to a specified amount.
     *
     * @param amount The new balance to set for the account.
     */
    void CheckingAccount::setAccountBalance(double amount)
    {
         setBalanceInternal(amount);
    }

    /**
     * @brief Retrieves the list of debit cards associated with this checking account.
     *
     * @return A constant reference to the vector of shared pointers to DebitCard objects.
     */
    const std::vector<std::shared_ptr<DebitCard>>& CheckingAccount::getDebitCards() const
    {
        return debit_cards_;
    }

    /**
     * @brief Retrieves the list of credit cards associated with this checking account.
     *
     * @return A constant reference to the vector of shared pointers to CreditCard objects.
     */
    const std::vector<std::shared_ptr<CreditCard>>& CheckingAccount::getCreditCards() const
    {
        return credit_cards_;
    }

    /**
     * @brief Issues a new credit card linked to this checking account.
     *
     * Creates a new CreditCard with the specified credit limit and activation status,
     * linking it to this checking account. Handles exceptions and logs errors if issuance fails.
     *
     * @param creditLimit The credit limit for the new credit card.
     * @param isActivated Whether the credit card is initially activated.
     */
    void CheckingAccount::issueCreditCard(double creditLimit, bool isActivated) {
        try {
            auto self = std::static_pointer_cast<CheckingAccount>(shared_from_this());
            addCreditCard(creditLimit, isActivated, self);
            std::cout << "Credit card issued for account #" << getAccountNumber() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to issue credit card: " << e.what() << std::endl;
        }
    }


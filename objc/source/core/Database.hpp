/*
 * Tencent is pleased to support the open source community by making
 * WCDB available.
 *
 * Copyright (C) 2017 THL A29 Limited, a Tencent company.
 * All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use
 * this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 *       https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef Database_hpp
#define Database_hpp

#include <WCDB/Abstract.h>
#include <WCDB/Factory.hpp>
#include <WCDB/HandlePool.hpp>
#include <WCDB/Migration.hpp>
#include <WCDB/Tag.hpp>
#include <WCDB/ThreadLocal.hpp>

namespace WCDB {

class Database : protected HandlePool {
#pragma mark - Initializer
public:
    Database(const std::string &path);
    Database() = delete;
    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

#pragma mark - Basic
public:
    void setTag(const Tag &tag);
    Tag getTag() const;

    bool canOpen();
    bool isOpened() const;
    using HandlePool::blockade;
    using ClosedCallback = HandlePool::DrainedCallback;
    void close(const ClosedCallback &onClosed);
    using HandlePool::unblockade;
    using HandlePool::isBlockaded;

protected:
    Tag m_tag;

#pragma mark - Handle
public:
    RecyclableHandle getHandle();
    bool execute(const Statement &statement);
    std::pair<bool, bool> tableExists(const TableOrSubquery &table);

protected:
    std::shared_ptr<Handle> generateHandle() override;
    bool handleWillConfigure(Handle *handle) override;
    void handleWillFlowBack(Handle *handle) override;

#pragma mark - Threaded
private:
    using ThreadedHandles = std::map<const Database *, RecyclableHandle>;
    static ThreadLocal<ThreadedHandles> &threadedHandles();
    static void
    markHandleAsThreaded(const Database *database, const RecyclableHandle &handle);
    static void markHandleAsUnthreaded(const Database *database);

    class ThreadedGuard {
    public:
        ThreadedGuard(const Database *database, const RecyclableHandle &handle);
        ~ThreadedGuard();

    private:
        bool m_isInTransactionBefore;
        const Database *m_database;
        RecyclableHandle m_handle;
    };

#pragma mark - Transaction
public:
    using TransactionCallback = Handle::TransactionCallback;

    bool beginTransaction();
    bool commitOrRollbackTransaction();
    void rollbackTransaction();
    bool runTransaction(const TransactionCallback &transaction);

    bool beginNestedTransaction();
    bool commitOrRollbackNestedTransaction();
    void rollbackNestedTransaction();
    bool runNestedTransaction(const TransactionCallback &transaction);

#pragma mark - Config
public:
    using HandlePool::setConfigs;
    using HandlePool::setConfig;
    using HandlePool::removeConfig;

#pragma mark - File
public:
    const std::string &getPath() const;
    std::string getSHMPath() const;
    std::string getWALPath() const;
    std::string getJournalPath() const;
    std::list<std::string> getPaths() const;

    bool moveFiles(const std::string &directory);
    bool moveFilesToDirectoryWithExtraFiles(const std::string &directory,
                                            const std::list<std::string> &extraFiles);
    bool removeFiles();
    std::pair<bool, size_t> getFilesSize();
    std::pair<bool, uint32_t> getIdentifier();

#pragma mark - Repair
public:
    std::string getFirstMaterialPath() const;
    std::string getLastMaterialPath() const;
    const std::string &getFactoryDirectory() const;

    typedef Repair::Factory::Filter BackupFilter;
    void filterBackup(const BackupFilter &tableShouldBeBackedup);
    bool backup();
    bool removeMaterials();

    bool deposit();
    bool removeDeposit();

    typedef Repair::FactoryRetriever::ProgressUpdateCallback RetrieveProgressCallback;
    double retrieve(const RetrieveProgressCallback &onProgressUpdate);
    bool canRetrieve() const;

private:
    bool retrieveRenewed();
    Repair::Factory m_factory;

#pragma mark - Recovery
public:
    enum RecoveryMode {
        Custom = 0,
        Remove = 1,
        Deposit = 2,
    };
    void setRecoveryMode(RecoveryMode mode);
    RecoveryMode getRecoverMode() const;

    typedef std::function<bool(Database *)> RecoverNotification;
    void setNotificationWhenRecovering(const RecoverNotification &notification);

    bool containsRecoverScheme() const;

    bool recover();

    bool isCorrupted() const;

private:
    RecoveryMode m_recoveryMode;
    RecoverNotification m_recoverNotification;

#pragma mark - Migration
public:
    void addMigrationInfo(const MigrationUserInfo &userInfo);

    typedef Migration::TableFilter MigrationTableFilter;
    void filterMigration(const MigrationTableFilter &filter);

    void asyncMigration();
    void stepMigration();

protected:
    bool rebindMigration(Handle *handle);
    Migration m_migration;

#pragma mark - Memory
public:
    using HandlePool::purge;

#pragma mark - Error
public:
    using HandlePool::getThreadedError;
};

} //namespace WCDB

#endif /* database_hpp */

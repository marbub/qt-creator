/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <projectstorageids.h>

#include <sqlitetable.h>
#include <sqlitetransaction.h>

#include <tuple>

namespace QmlDesigner {

template<typename Database>
class ProjectStorage
{
public:
    template<int ResultCount>
    using ReadStatement = typename Database::template ReadStatement<ResultCount>;
    template<int ResultCount>
    using ReadWriteStatement = typename Database::template ReadWriteStatement<ResultCount>;
    using WriteStatement = typename Database::WriteStatement;

    ProjectStorage(Database &database, bool isInitialized)
        : database{database}
        , initializer{database, isInitialized}
    {}

    template<typename Container>
    InternalTypeId upsertType(Utils::SmallStringView name,
                              InternalTypeId prototype,
                              TypeAccessSemantics accessSemantics,
                              const Container &qualifiedNames)
    {
        Sqlite::ImmediateTransaction transaction{database};

        auto internalTypeId = upsertTypeStatement.template value<InternalTypeId>(
            name, static_cast<long long>(accessSemantics), prototype.id);

        for (Utils::SmallStringView qualifiedName : qualifiedNames)
            upsertQualifiedTypeNameStatement.write(qualifiedName, internalTypeId->id);

        transaction.commit();

        return *internalTypeId;
    }

    InternalPropertyDeclarationId upsertPropertyDeclaration(InternalTypeId typeId,
                                                            Utils::SmallStringView name,
                                                            InternalTypeId propertyTypeId)
    {
        Sqlite::ImmediateTransaction transaction{database};

        auto propertyDeclarationId = upsertPropertyDeclarationStatement.template value<
            InternalPropertyDeclarationId>(typeId.id, name, propertyTypeId.id);

        transaction.commit();

        return *propertyDeclarationId;
    }

    InternalPropertyDeclarationId fetchPropertyDeclarationByTypeIdAndName(InternalTypeId typeId,
                                                                          Utils::SmallStringView name)
    {
        Sqlite::DeferredTransaction transaction{database};

        auto propertyDeclarationId = selectPropertyDeclarationByTypeIdAndNameStatement
                                         .template value<InternalPropertyDeclarationId>(typeId.id,
                                                                                        name);

        transaction.commit();

        if (propertyDeclarationId)
            return *propertyDeclarationId;

        return InternalPropertyDeclarationId{};
    }

    InternalTypeId fetchTypeIdByQualifiedName(Utils::SmallStringView name)
    {
        Sqlite::DeferredTransaction transaction{database};

        auto typeId = selectTypeIdByQualifiedNameStatement.template value<InternalTypeId>(name);

        transaction.commit();

        if (typeId)
            return *typeId;

        return InternalTypeId{};
    }

    bool fetchIsProtype(InternalTypeId type, InternalTypeId prototype)
    {
        Sqlite::DeferredTransaction transaction{database};

        auto typeId = selectPrototypeIdStatement.template value<InternalTypeId>(type.id, prototype.id);

        transaction.commit();

        return bool(typeId);
    }

private:
    class Initializer
    {
    public:
        Initializer(Database &database, bool isInitialized)
        {
            if (!isInitialized) {
                Sqlite::ExclusiveTransaction transaction{database};

                createTypesTable(database);
                createQualifiedTypeNamesTable(database);
                createPropertyDeclarationsTable(database);
                createEnumValuesTable(database);
                createMethodsTable(database);
                createSignalsTable(database);

                transaction.commit();

                database.walCheckpointFull();
            }
        }

        void createPropertyDeclarationsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("propertyDeclarations");
            table.addColumn("propertyDeclarationId",
                            Sqlite::ColumnType::Integer,
                            {Sqlite::PrimaryKey{}});
            auto &typeIdColumn = table.addColumn("typeId");
            auto &nameColumn = table.addColumn("name");
            table.addColumn("propertyTypeId");

            table.addUniqueIndex({typeIdColumn, nameColumn});

            table.initialize(database);
        }

        void createTypesTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("types");
            table.addColumn("typeId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &nameColumn = table.addColumn("name");
            table.addColumn("accessSemantics");
            table.addColumn("prototype");
            table.addColumn("defaultProperty");

            table.addUniqueIndex({nameColumn});

            table.initialize(database);
        }

        void createQualifiedTypeNamesTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setUseWithoutRowId(true);
            table.setName("qualifiedTypeNames");
            auto &qualifiedNameColumn = table.addColumn("qualifiedName");
            table.addColumn("typeId");

            table.addPrimaryKeyContraint({qualifiedNameColumn});

            table.initialize(database);
        }

        void createEnumValuesTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("enumerationValues");
            table.addColumn("enumerationValueId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &enumIdColumn = table.addColumn("typeId");
            auto &nameColumn = table.addColumn("name");

            table.addUniqueIndex({enumIdColumn, nameColumn});

            table.initialize(database);
        }

        void createMethodsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("methods");
            table.addColumn("methodId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &nameColumn = table.addColumn("name");

            table.addUniqueIndex({nameColumn});

            table.initialize(database);
        }

        void createSignalsTable(Database &database)
        {
            Sqlite::Table table;
            table.setUseIfNotExists(true);
            table.setName("signals");
            table.addColumn("signalId", Sqlite::ColumnType::Integer, {Sqlite::PrimaryKey{}});
            auto &nameColumn = table.addColumn("name");

            table.addUniqueIndex({nameColumn});

            table.initialize(database);
        }
    };

public:
    Database &database;
    Initializer initializer;
    ReadWriteStatement<1> upsertTypeStatement{
        "INSERT INTO types(name,  accessSemantics, prototype) VALUES(?1, ?2, nullif(?3, -1)) ON "
        "CONFLICT DO UPDATE SET "
        "prototype=excluded.prototype, accessSemantics=excluded.accessSemantics RETURNING typeId",
        database};
    mutable ReadStatement<1> selectTypeIdByQualifiedNameStatement{
        "SELECT typeId FROM qualifiedTypeNames WHERE qualifiedName=?", database};
    mutable ReadStatement<1> selectPrototypeIdStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT prototype FROM types JOIN typeSelection USING(typeId)) "
        "SELECT typeId FROM typeSelection WHERE typeId=?2 LIMIT 1",
        database};
    ReadWriteStatement<1> upsertPropertyDeclarationStatement{
        "INSERT INTO propertyDeclarations(typeId, name, propertyTypeId) VALUES(?1, ?2, ?3) ON "
        "CONFLICT DO UPDATE SET "
        "typeId=excluded.typeId, name=excluded.name, propertyTypeId=excluded.propertyTypeId  "
        "RETURNING propertyDeclarationId",
        database};
    mutable ReadStatement<1> selectPropertyDeclarationByTypeIdAndNameStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT prototype FROM types JOIN typeSelection USING(typeId)) "
        "SELECT propertyDeclarationId FROM propertyDeclarations JOIN typeSelection USING(typeId) "
        "  WHERE name=?2 LIMIT 1",
        database};
    WriteStatement upsertQualifiedTypeNameStatement{
        "INSERT INTO qualifiedTypeNames(qualifiedName,  typeId) VALUES(?1, ?2) ON CONFLICT DO "
        "UPDATE SET typeId=excluded.typeId",
        database};
    mutable ReadStatement<1> selectAccessSemanticsStatement{
        "SELECT typeId FROM qualifiedTypeNames WHERE qualifiedName=?", database};
    mutable ReadStatement<1> selectPrototypeIdsStatement{
        "WITH RECURSIVE "
        "  typeSelection(typeId) AS ("
        "      VALUES(?1) "
        "    UNION ALL "
        "      SELECT prototype FROM types JOIN typeSelection USING(typeId)) "
        "SELECT typeId FROM typeSelection",
        database};
};

} // namespace QmlDesigner

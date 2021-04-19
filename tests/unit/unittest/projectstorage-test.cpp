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

#include "googletest.h"

#include "sqlitedatabasemock.h"

#include <metainfo/projectstorage.h>
#include <modelnode.h>
#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

namespace {

using QmlDesigner::InternalPropertyDeclarationId;
using QmlDesigner::InternalTypeId;
using QmlDesigner::TypeAccessSemantics;

class ProjectStorage : public testing::Test
{
protected:
    using SqliteModelStorage = QmlDesigner::ProjectStorage<SqliteDatabaseMock>;
    template<int ResultCount>
    using ReadStatement = SqliteModelStorage::ReadStatement<ResultCount>;
    using WriteStatement = SqliteModelStorage::WriteStatement;
    template<int ResultCount>
    using ReadWriteStatement = SqliteDatabaseMock::ReadWriteStatement<ResultCount>;

    NiceMock<SqliteDatabaseMock> databaseMock;
    SqliteModelStorage storage{databaseMock, true};
    ReadWriteStatement<1> &upsertTypeStatement = storage.upsertTypeStatement;
    ReadStatement<1> &selectTypeIdByQualifiedNameStatement = storage.selectTypeIdByQualifiedNameStatement;
    ReadWriteStatement<1> &upsertPropertyDeclarationStatement = storage.upsertPropertyDeclarationStatement;
    ReadStatement<1> &selectPropertyDeclarationByTypeIdAndNameStatement = storage.selectPropertyDeclarationByTypeIdAndNameStatement;
    WriteStatement &upsertQualifiedTypeNameStatement = storage.upsertQualifiedTypeNameStatement;
};

TEST_F(ProjectStorage, InsertTypeCalls)
{
    InSequence s;
    InternalTypeId prototypeId{3};
    InternalTypeId newTypeId{11};

    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertTypeStatement,
                valueReturnsInternalTypeId(Eq("QObject"),
                                           TypedEq<long long>(static_cast<long long>(
                                               TypeAccessSemantics::Reference)),
                                           Eq(prototypeId.id)))
        .WillOnce(Return(newTypeId));
    EXPECT_CALL(upsertQualifiedTypeNameStatement,
                write(TypedEq<Utils::SmallStringView>("Qml.Object"), TypedEq<long long>(newTypeId.id)));
    EXPECT_CALL(upsertQualifiedTypeNameStatement,
                write(TypedEq<Utils::SmallStringView>("Quick.Object"),
                      TypedEq<long long>(newTypeId.id)));
    EXPECT_CALL(databaseMock, commit);

    storage.upsertType("QObject",
                       prototypeId,
                       TypeAccessSemantics::Reference,
                       std::vector<Utils::SmallString>{"Qml.Object", "Quick.Object"});
}

TEST_F(ProjectStorage, InsertTypeReturnsInternalPropertyId)
{
    InternalTypeId prototypeId{3};
    InternalTypeId newTypeId{11};
    ON_CALL(upsertTypeStatement, valueReturnsInternalTypeId(Eq("QObject"), _, Eq(prototypeId.id)))
        .WillByDefault(Return(newTypeId));

    auto internalId = storage.upsertType("QObject",
                                         prototypeId,
                                         TypeAccessSemantics::Reference,
                                         std::vector<Utils::SmallString>{});

    ASSERT_THAT(internalId, Eq(newTypeId));
}

TEST_F(ProjectStorage, FetchTypeIdByName)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectTypeIdByQualifiedNameStatement,
                valueReturnsInternalTypeId(TypedEq<Utils::SmallStringView>("boo")));
    EXPECT_CALL(databaseMock, commit);

    storage.fetchTypeIdByQualifiedName("boo");
}

TEST_F(ProjectStorage, UpsertPropertyDeclaration)
{
    InSequence s;

    EXPECT_CALL(databaseMock, immediateBegin());
    EXPECT_CALL(upsertPropertyDeclarationStatement,
                valueReturnsInternalPropertyDeclarationId(TypedEq<long long>(11),
                                                          TypedEq<Utils::SmallStringView>("boo"),
                                                          TypedEq<long long>(33)))
        .WillOnce(Return(InternalPropertyDeclarationId{3}));
    EXPECT_CALL(databaseMock, commit);

    storage.upsertPropertyDeclaration(InternalTypeId{11}, "boo", InternalTypeId{33});
}

TEST_F(ProjectStorage, FetchPropertyDeclarationByTypeIdAndName)
{
    InSequence s;

    EXPECT_CALL(databaseMock, deferredBegin());
    EXPECT_CALL(selectPropertyDeclarationByTypeIdAndNameStatement,
                valueReturnsInternalPropertyDeclarationId(TypedEq<long long>(11),
                                                          TypedEq<Utils::SmallStringView>("boo")));
    EXPECT_CALL(databaseMock, commit);

    storage.fetchPropertyDeclarationByTypeIdAndName(InternalTypeId{11}, "boo");
}

class ProjectStorageSlowTest : public testing::Test
{
protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    QmlDesigner::ProjectStorage<Sqlite::Database> storage{database, database.isInitialized()};
};

TEST_F(ProjectStorageSlowTest, FetchTypeIdByName)
{
    storage.upsertType("Yi",
                       InternalTypeId{},
                       TypeAccessSemantics::Reference,
                       std::vector<Utils::SmallString>{"Qml.Yi"});
    auto internalTypeId = storage.upsertType("Er",
                                             InternalTypeId{},
                                             TypeAccessSemantics::Reference,
                                             std::vector<Utils::SmallString>{"Qml.Er"});
    storage.upsertType("San",
                       InternalTypeId{},
                       TypeAccessSemantics::Reference,
                       std::vector<Utils::SmallString>{"Qml.San"});

    auto id = storage.fetchTypeIdByQualifiedName("Qml.Er");

    ASSERT_THAT(id, internalTypeId);
}

TEST_F(ProjectStorageSlowTest, InsertType)
{
    auto internalTypeId = storage.upsertType("Yi",
                                             InternalTypeId{},
                                             TypeAccessSemantics::Reference,
                                             std::vector<Utils::SmallString>{"Qml.Yi"});

    ASSERT_THAT(storage.fetchTypeIdByQualifiedName("Qml.Yi"), internalTypeId);
}

TEST_F(ProjectStorageSlowTest, UpsertType)
{
    auto internalTypeId = storage.upsertType("Yi",
                                             InternalTypeId{},
                                             TypeAccessSemantics::Reference,
                                             std::vector<Utils::SmallString>{"Qml.Yi"});

    auto internalTypeId2 = storage.upsertType("Yi",
                                              InternalTypeId{},
                                              TypeAccessSemantics::Reference,
                                              std::vector<Utils::SmallString>{"Qml.Yi"});

    ASSERT_THAT(internalTypeId2, internalTypeId);
}

TEST_F(ProjectStorageSlowTest, InsertTypeIdAreUnique)
{
    auto internalTypeId = storage.upsertType("Yi",
                                             InternalTypeId{},
                                             TypeAccessSemantics::Reference,
                                             std::vector<Utils::SmallString>{"Qml.Yi"});
    auto internalTypeId2 = storage.upsertType("Er",
                                              InternalTypeId{},
                                              TypeAccessSemantics::Reference,
                                              std::vector<Utils::SmallString>{"Qml.Er"});

    ASSERT_TRUE(internalTypeId != internalTypeId2);
}

TEST_F(ProjectStorageSlowTest, IsConvertibleTypeToBase)
{
    auto baseId = storage.upsertType("Base",
                                     InternalTypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Quick.Item"});

    auto isConvertible = storage.fetchIsProtype(itemId, baseId);

    ASSERT_TRUE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsConvertibleTypeToSameType)
{
    auto baseId = storage.upsertType("Base",
                                     InternalTypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Quick.Item"});

    auto isConvertible = storage.fetchIsProtype(itemId, itemId);

    ASSERT_TRUE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsConvertibleTypeToSomeTypeInTheMiddle)
{
    auto baseId = storage.upsertType("Base",
                                     InternalTypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Quick.Item"});

    auto isConvertible = storage.fetchIsProtype(itemId, objectId);

    ASSERT_TRUE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsNotConvertibleToUnrelatedType)
{
    auto unrelatedId = storage.upsertType("Base",
                                          InternalTypeId{},
                                          TypeAccessSemantics::Reference,
                                          std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       InternalTypeId{},
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});
    auto itemId = storage.upsertType("QQuickItem",
                                     objectId,
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Quick.Item"});

    auto isConvertible = storage.fetchIsProtype(itemId, unrelatedId);

    ASSERT_FALSE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsNotConvertibleToCousineType)
{
    auto baseId = storage.upsertType("Base",
                                     InternalTypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});
    auto itemId = storage.upsertType("QQuickItem",
                                     baseId,
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Quick.Item"});

    auto isConvertible = storage.fetchIsProtype(itemId, objectId);

    ASSERT_FALSE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, IsNotConvertibleToDerivedType)
{
    auto baseId = storage.upsertType("Base",
                                     InternalTypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Base"});
    auto objectId = storage.upsertType("QObject",
                                       baseId,
                                       TypeAccessSemantics::Reference,
                                       std::vector<Utils::SmallString>{"Qml.Object"});

    auto isConvertible = storage.fetchIsProtype(baseId, objectId);

    ASSERT_FALSE(isConvertible);
}

TEST_F(ProjectStorageSlowTest, InsertPropertyDeclaration)
{
    auto typeId = storage.upsertType("QObject",
                                     InternalTypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Object"});
    auto propertyTypeId = storage.upsertType("double",
                                             InternalTypeId{},
                                             TypeAccessSemantics::Value,
                                             std::vector<Utils::SmallString>{"Qml.doube"});

    auto propertyDeclarationId = storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    ASSERT_THAT(storage.fetchPropertyDeclarationByTypeIdAndName(typeId, "foo"), propertyDeclarationId);
}

TEST_F(ProjectStorageSlowTest, UpsertPropertyDeclaration)
{
    auto typeId = storage.upsertType("QObject",
                                     InternalTypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Object"});
    auto propertyTypeId = storage.upsertType("double",
                                             InternalTypeId{},
                                             TypeAccessSemantics::Value,
                                             std::vector<Utils::SmallString>{"Qml.doube"});
    auto propertyDeclarationId = storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    auto propertyDeclarationId2 = storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    ASSERT_THAT(propertyDeclarationId2, Eq(propertyDeclarationId));
}

TEST_F(ProjectStorageSlowTest, FetchPropertyDeclarationByTypeIdAndNameFromSameType)
{
    auto typeId = storage.upsertType("QObject",
                                     InternalTypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Object"});
    auto propertyTypeId = storage.upsertType("double",
                                             InternalTypeId{},
                                             TypeAccessSemantics::Value,
                                             std::vector<Utils::SmallString>{"Qml.doube"});
    auto propertyDeclarationId = storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    auto id = storage.fetchPropertyDeclarationByTypeIdAndName(typeId, "foo");

    ASSERT_THAT(id, propertyDeclarationId);
}

TEST_F(ProjectStorageSlowTest, CannotFetchPropertyDeclarationByTypeIdAndNameForNonExistingProperty)
{
    auto typeId = storage.upsertType("QObject",
                                     InternalTypeId{},
                                     TypeAccessSemantics::Reference,
                                     std::vector<Utils::SmallString>{"Qml.Object"});
    auto propertyTypeId = storage.upsertType("double",
                                             InternalTypeId{},
                                             TypeAccessSemantics::Value,
                                             std::vector<Utils::SmallString>{"Qml.doube"});
    storage.upsertPropertyDeclaration(typeId, "foo", propertyTypeId);

    auto id = storage.fetchPropertyDeclarationByTypeIdAndName(typeId, "bar");

    ASSERT_FALSE(id.isValid());
}

TEST_F(ProjectStorageSlowTest, FetchPropertyDeclarationByTypeIdAndNameFromDerivedType)
{
    auto baseTypeId = storage.upsertType("QObject",
                                         InternalTypeId{},
                                         TypeAccessSemantics::Reference,
                                         std::vector<Utils::SmallString>{"Qml.Object"});
    auto propertyTypeId = storage.upsertType("double",
                                             InternalTypeId{},
                                             TypeAccessSemantics::Value,
                                             std::vector<Utils::SmallString>{"Qml.doube"});
    auto derivedTypeId = storage.upsertType("Derived",
                                            baseTypeId,
                                            TypeAccessSemantics::Reference,
                                            std::vector<Utils::SmallString>{"Qml.Derived"});
    auto propertyDeclarationId = storage.upsertPropertyDeclaration(baseTypeId, "foo", propertyTypeId);

    auto id = storage.fetchPropertyDeclarationByTypeIdAndName(derivedTypeId, "foo");

    ASSERT_THAT(id, propertyDeclarationId);
}

TEST_F(ProjectStorageSlowTest, FetchPropertyDeclarationByTypeIdAndNameFromBaseType)
{
    auto baseTypeId = storage.upsertType("QObject",
                                         InternalTypeId{},
                                         TypeAccessSemantics::Reference,
                                         std::vector<Utils::SmallString>{"Qml.Object"});
    auto propertyTypeId = storage.upsertType("double",
                                             InternalTypeId{},
                                             TypeAccessSemantics::Value,
                                             std::vector<Utils::SmallString>{"Qml.doube"});
    auto derivedTypeId = storage.upsertType("Derived",
                                            baseTypeId,
                                            TypeAccessSemantics::Reference,
                                            std::vector<Utils::SmallString>{"Qml.Derived"});
    storage.upsertPropertyDeclaration(derivedTypeId, "foo", propertyTypeId);

    auto id = storage.fetchPropertyDeclarationByTypeIdAndName(baseTypeId, "foo");

    ASSERT_FALSE(id.isValid());
}

} // namespace

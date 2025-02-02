// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#include <QtSql/QtSql>

#include "../../../../auto/sql/kernel/qsqldatabase/tst_databases.h"

const QString qtest(qTableName("qtest", __FILE__, QSqlDatabase()));

class tst_QSqlQuery : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

public slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

private slots:
    void benchmark_data() { generic_data(); }
    void benchmark();
    void benchmarkSelectPrepared_data() { generic_data(); }
    void benchmarkSelectPrepared();

private:
    // returns all database connections
    void generic_data(const QString &engine=QString());
    void dropTestTables( QSqlDatabase db );
    void createTestTables( QSqlDatabase db );
    void populateTestTables( QSqlDatabase db );

    tst_Databases dbs;
};

QTEST_MAIN(tst_QSqlQuery)

void tst_QSqlQuery::initTestCase()
{
    dbs.open();

    for (const auto &dbName : std::as_const(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE( db );
        dropTestTables( db ); //in case of leftovers
        createTestTables( db );
        populateTestTables( db );
    }
}

void tst_QSqlQuery::cleanupTestCase()
{
    for (const auto &dbName : std::as_const(dbs.dbNames)) {
        QSqlDatabase db = QSqlDatabase::database(dbName);
        CHECK_DATABASE( db );
        dropTestTables( db );
    }

    dbs.close();
}

void tst_QSqlQuery::init()
{
}

void tst_QSqlQuery::cleanup()
{
    QFETCH( QString, dbName );
    QSqlDatabase db = QSqlDatabase::database( dbName );
    CHECK_DATABASE( db );
    const QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);

    if ( QTest::currentTestFunction() == QLatin1String( "numRowsAffected" )
            || QTest::currentTestFunction() == QLatin1String( "transactions" )
            || QTest::currentTestFunction() == QLatin1String( "size" )
            || QTest::currentTestFunction() == QLatin1String( "isActive" )
            || QTest::currentTestFunction() == QLatin1String( "lastInsertId" ) ) {
        populateTestTables( db );
    }

    if (QTest::currentTestFailed() && (dbType == QSqlDriver::Oracle || db.driverName().startsWith("QODBC"))) {
        //since Oracle ODBC totally craps out on error, we init again
        db.close();
        db.open();
    }
}

void tst_QSqlQuery::generic_data(const QString& engine)
{
    if ( dbs.fillTestTable(engine) == 0 ) {
        if (engine.isEmpty())
           QSKIP( "No database drivers are available in this Qt configuration");
        else
           QSKIP( (QString("No database drivers of type %1 are available in this Qt configuration").arg(engine)).toLocal8Bit());
    }
}

void tst_QSqlQuery::dropTestTables( QSqlDatabase db )
{
    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    QStringList tablenames;
    // drop all the table in case a testcase failed
    tablenames <<  qtest
               << qTableName("qtest_null", __FILE__, db)
               << qTableName("qtest_blob", __FILE__, db)
               << qTableName("qtest_bittest", __FILE__, db)
               << qTableName("qtest_nullblob", __FILE__, db)
               << qTableName("qtest_rawtest", __FILE__, db)
               << qTableName("qtest_precision", __FILE__, db)
               << qTableName("qtest_prepare", __FILE__, db)
               << qTableName("qtestj1", __FILE__, db)
               << qTableName("qtestj2", __FILE__, db)
               << qTableName("char1Select", __FILE__, db)
               << qTableName("char1SU", __FILE__, db)
               << qTableName("qxmltest", __FILE__, db)
               << qTableName("qtest_exerr", __FILE__, db)
               << qTableName("qtest_empty", __FILE__, db)
               << qTableName("clobby", __FILE__, db)
               << qTableName("bindtest", __FILE__, db)
               << qTableName("more_results", __FILE__, db)
               << qTableName("blobstest", __FILE__, db)
               << qTableName("oraRowId", __FILE__, db)
               << qTableName("qtest_batch", __FILE__, db)
               << qTableName("bug6421", __FILE__, db).toUpper()
               << qTableName("bug5765", __FILE__, db)
               << qTableName("bug6852", __FILE__, db)
               << qTableName("qtest_lockedtable", __FILE__, db)
               << qTableName("Planet", __FILE__, db)
               << qTableName("task_250026", __FILE__, db)
               << qTableName("task_234422", __FILE__, db)
               << qTableName("test141895", __FILE__, db)
               << qTableName("qtest_oraOCINumber", __FILE__, db);

    if (dbType == QSqlDriver::PostgreSQL)
        tablenames << qTableName("task_233829", __FILE__, db);

    if (dbType == QSqlDriver::SQLite)
        tablenames << qTableName("record_sqlite", __FILE__, db);

    if (dbType == QSqlDriver::MSSqlServer || dbType == QSqlDriver::Oracle)
        tablenames << qTableName("qtest_longstr", __FILE__, db);

    QSqlQuery q(db);
    if (dbType == QSqlDriver::MSSqlServer)
        q.exec("DROP PROCEDURE " + qTableName("test141895_proc", __FILE__, db));

    if (dbType == QSqlDriver::MySqlServer)
        q.exec("DROP PROCEDURE IF EXISTS "+qTableName("bug6852_proc", __FILE__, db));

    tst_Databases::safeDropTables( db, tablenames );

    if (dbType == QSqlDriver::Oracle)
        q.exec("DROP PACKAGE " + qTableName("pkg", __FILE__, db));
}

void tst_QSqlQuery::createTestTables( QSqlDatabase db )
{
    const QString qtestNull = qTableName("qtest_null", __FILE__, db);
    QSqlQuery q( db );
    QSqlDriver::DbmsType dbType = tst_Databases::getDatabaseType(db);
    if (dbType == QSqlDriver::MySqlServer)
        // ### stupid workaround until we find a way to hardcode this
        // in the MySQL server startup script
        q.exec( "set table_type=innodb" );
    else if (dbType == QSqlDriver::PostgreSQL)
        QVERIFY_SQL( q, exec("set client_min_messages='warning'"));

    if (dbType == QSqlDriver::PostgreSQL)
        QVERIFY_SQL( q, exec( "create table " + qtest + " (id serial NOT NULL, t_varchar varchar(20), t_char char(20), primary key(id)) WITH OIDS" ) );
    else
        QVERIFY_SQL( q, exec( "create table " + qtest + " (id int "+tst_Databases::autoFieldName(db) +" NOT NULL, t_varchar varchar(20), t_char char(20), primary key(id))" ) );

    if (dbType == QSqlDriver::MSSqlServer || dbType == QSqlDriver::Sybase)
        QVERIFY_SQL(q, exec("create table " + qtestNull + " (id int null, t_varchar varchar(20) null)"));
    else
        QVERIFY_SQL(q, exec("create table " + qtestNull + " (id int, t_varchar varchar(20))"));
}

void tst_QSqlQuery::populateTestTables( QSqlDatabase db )
{
    QSqlQuery q( db );
    const QString qtest_null(qTableName("qtest_null", __FILE__, db));
    q.exec( "delete from " + qtest );
    QVERIFY_SQL( q, exec( "insert into " + qtest + " values (1, 'VarChar1', 'Char1')" ) );
    QVERIFY_SQL( q, exec( "insert into " + qtest + " values (2, 'VarChar2', 'Char2')" ) );
    QVERIFY_SQL( q, exec( "insert into " + qtest + " values (3, 'VarChar3', 'Char3')" ) );
    QVERIFY_SQL( q, exec( "insert into " + qtest + " values (4, 'VarChar4', 'Char4')" ) );
    QVERIFY_SQL( q, exec( "insert into " + qtest + " values (5, 'VarChar5', 'Char5')" ) );

    q.exec( "delete from " + qtest_null );
    QVERIFY_SQL( q, exec( "insert into " + qtest_null + " values (0, NULL)" ) );
    QVERIFY_SQL( q, exec( "insert into " + qtest_null + " values (1, 'n')" ) );
    QVERIFY_SQL( q, exec( "insert into " + qtest_null + " values (2, 'i')" ) );
    QVERIFY_SQL( q, exec( "insert into " + qtest_null + " values (3, NULL)" ) );
}

void tst_QSqlQuery::benchmark()
{
    QFETCH( QString, dbName );
    QSqlDatabase db = QSqlDatabase::database( dbName );
    CHECK_DATABASE( db );
    QSqlQuery q(db);
    TableScope ts(db, "benchmark", __FILE__);

    QVERIFY_SQL(q, exec("CREATE TABLE " + ts.tableName() + "(\n"
                        "MainKey INT NOT NULL,\n"
                        "OtherTextCol VARCHAR(45) NOT NULL,\n"
                        "PRIMARY KEY(`MainKey`))"));

    int i=1;

    QBENCHMARK {
        const QString num = QString::number(i);
        QVERIFY_SQL(q, exec("INSERT INTO " + ts.tableName() + " VALUES(" + num + ", \"Value" + num + "\")"));
        i++;
    }
}

void tst_QSqlQuery::benchmarkSelectPrepared()
{
    QFETCH( QString, dbName );
    QSqlDatabase db = QSqlDatabase::database(dbName);
    CHECK_DATABASE(db);
    QSqlQuery q(db);
    TableScope ts(db, "benchmark", __FILE__);

    QVERIFY_SQL(q, exec("CREATE TABLE " + ts.tableName() + "(id INT NOT NULL)"));

    const int NUM_ROWS = 1000;
    int expectedSum = 0;
    QString fillQuery = "INSERT INTO " + ts.tableName() + " VALUES (0)";
    for (int i = 1; i < NUM_ROWS; ++i) {
        fillQuery += ", (" + QString::number(i) + QLatin1Char(')');
        expectedSum += i;
    }
    QVERIFY_SQL(q, exec(fillQuery));

    QVERIFY_SQL(q, prepare("SELECT id FROM " + ts.tableName()));
    QBENCHMARK {
        QVERIFY_SQL(q, exec());
        int sum = 0;

        while (q.next())
            sum += q.value(0).toInt();

        QCOMPARE(sum, expectedSum);
    }
}

#include "main.moc"

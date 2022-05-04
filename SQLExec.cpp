/**
 * @file SQLExec.cpp - implementation of SQLExec class
 * @author Kevin Lundeen
 * @see "Seattle University, CPSC5300, Spring 2022"
 */
#include "SQLExec.h"

using namespace std;
using namespace hsql;

// define static data
Tables *SQLExec::tables = nullptr;

// make query result be printable
ostream &operator<<(ostream &out, const QueryResult &qres) {
    if (qres.column_names != nullptr) {
        for (auto const &column_name: *qres.column_names)
            out << column_name << " ";
        out << endl << "+";
        for (unsigned int i = 0; i < qres.column_names->size(); i++)
            out << "----------+";
        out << endl;
        for (auto const &row: *qres.rows) {
            for (auto const &column_name: *qres.column_names) {
                Value value = row->at(column_name);
                switch (value.data_type) {
                    case ColumnAttribute::INT:
                        out << value.n;
                        break;
                    case ColumnAttribute::TEXT:
                        out << "\"" << value.s << "\"";
                        break;
                    default:
                        out << "???";
                }
                out << " ";
            }
            out << endl;
        }
    }
    out << qres.message;
    return out;
}

QueryResult::~QueryResult() {
    // FIXME
}


QueryResult *SQLExec::execute(const SQLStatement *statement) {
    // initialize _tables table, if not yet present
    if (SQLExec::tables == nullptr){
        SQLExec::tables = new Tables();
    }

    try {
        switch (statement->type()) {
            case kStmtCreate:
                return create((const CreateStatement *) statement);
            case kStmtDrop:
                return drop((const DropStatement *) statement);
            case kStmtShow:
                return show((const ShowStatement *) statement);
            default:
                return new QueryResult("not implemented");
        }
    } catch (DbRelationError &e) {
        throw SQLExecError(string("DbRelationError: ") + e.what());
    }
}

void SQLExec::column_definition(const ColumnDefinition *col, Identifier &column_name, ColumnAttribute &column_attribute) {
    
}

QueryResult *SQLExec::create(const CreateStatement *statement) {
    ValueDict row;
    Identifier table_name = statement->tableName;
    row["table_name"] = table_name;
    SQLExec::tables->insert(&row);

    // Update columns schema


    return new QueryResult("created " + table_name); 
}

// DROP ...
QueryResult *SQLExec::drop(const DropStatement *statement) {
    if (statement->type != hsql::DropStatement::kTable){
        throw SQLExecError("Unrecognized DROP type");
    }

    if (Value(statement->name) == Tables::TABLE_NAME || Value(statement->name) == Columns::TABLE_NAME){
        throw SQLExecError("Cannot drop a schema table");
    }

    ValueDict where;
    Identifier table_name = statement->name;
    
    // Get table to drop
    DbRelation &table = SQLExec::tables->get_table(table_name);   
    where["table_name"] = Value(table_name);

    Handles *handles = table.select(&where);

    for (auto const &handle: *handles){
        table.del(handle);
    }
    delete handles;

    table.drop();
    SQLExec::tables->del(*SQLExec::tables->select(&where)->begin());

    return new QueryResult(string("dropped ") + table_name);
}

QueryResult *SQLExec::show(const ShowStatement *statement) {
    // checks for 2 conditions -> show tables and show columns
    switch (statement->type) {
        case ShowStatement::kTables:
            return show_tables();
        case ShowStatement::kColumns:
            return show_columns(statement);
        default:
            return new QueryResult("not implemented");
    }
}

QueryResult *SQLExec::show_tables() {
    // get all tables
    Handles *handles = SQLExec::tables->select();
    ColumnNames *columnNames = new ColumnNames();
    ValueDicts *rows = new ValueDicts();
    int count = 0;

    columnNames->push_back("table_name");
    
    for (auto const &handle: *handles){
        ValueDict *row = SQLExec::tables->project(handle, columnNames);
        Identifier table = row->at("table_name").s;
        if (table == Tables::TABLE_NAME || table == Columns::TABLE_NAME){
            continue;
        }
        count++;
        rows->push_back(row);
    }
    delete handles;

    return new QueryResult(columnNames, new ColumnAttributes(), rows, "successfully returned " + to_string(count) + " rows"); 
}

QueryResult *SQLExec::show_columns(const ShowStatement *statement) {   
    Identifier table_name = statement->tableName;
    DbRelation &table = SQLExec::tables->get_table(table_name);   

    ColumnNames *columnNames = new ColumnNames();
    ColumnAttributes *columnAttributes = new ColumnAttributes();
    ValueDicts *rows = new ValueDicts();

    int count = 0;
    Handles *handles = table.select(); 

    SQLExec::tables->get_columns(table_name, *columnNames, *columnAttributes);

    for (auto const &handle: *handles) {
        ValueDict *row = table.project(handle, columnNames); 
        Identifier table = row->at("table_name").s;
        if (table == table_name){
            rows->push_back(row);
            count++;
        }
    }
    delete handles;

    return new QueryResult(columnNames, columnAttributes, rows,  "successfully returned " + to_string(count) + " rows");
}


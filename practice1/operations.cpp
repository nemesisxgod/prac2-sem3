#include "header.h"

//копирование названий колонок
void copyColNames(const filesystem::path& file_read, const filesystem::path& file_write) {
    string columns;
    ifstream filer(file_read);
    if (!filer.is_open()) {
        throw runtime_error("cannot open file.");
    }
    filer >> columns;
    filer.close();
    ofstream filew(file_write);
    if (!filew.is_open()) {
        throw runtime_error("cannot open file.");
    }
    filew << columns << endl;
    filew.close();
}

void insert(string command, TableJson& jsontb) {
    istringstream cmnd(command);
    string mes;
    cmnd >> mes;
    cmnd >> mes;
    if (mes!="INTO") {
        throw runtime_error("invalid data entry.");
    }

    cmnd >> mes;
    string table = mes;
    if (!tableExist(table, jsontb.head)) {
        throw runtime_error("table is not exist.");
    }
    if (isLocked(table, jsontb.scheme)) {
        throw runtime_error("file is blocked for edit.");
    }

    cmnd >> mes;
    if (mes!="VALUES") {
        throw runtime_error("invalid data entry.");
    }
    string values;
    while (cmnd >> mes) {
        values += mes;
    }
    if (values.front() != '(' || values.back()!= ')') {
        throw runtime_error("invalid data entry.");
    }
    for (size_t i=1; i < values.size(); i++) {
        if (values[i] == '\'') {
            i++;
            while (values[i]!='\'') {
                i++;
                if (values[i] == ',') {
                    throw runtime_error("not all values are enclosed in quotes.");
                }
            }
        }
    }

    tableLocker(table, jsontb.scheme);
    filesystem::path pathPk = filesystem::current_path() / jsontb.scheme / table / (table + "_pk_sequence.txt");
    ifstream file(pathPk);
    if (!file.is_open()) {
      throw runtime_error("cannot open file.");
    }
    int pk;
    file >> pk;
    file.close();
    pk++;
    ofstream fileO(pathPk);
    if (!fileO.is_open()) {
      throw runtime_error("cannot open file.");
    }
    fileO << pk;
    fileO.close();

    int csvCount = 1;
    while (true) {
        filesystem::path pathCSV = filesystem::current_path() / jsontb.scheme / table / (to_string(csvCount) + ".csv");
        ifstream file(pathCSV);
        if (!file.is_open()) {
            ofstream fileO(pathCSV);
            if (!fileO.is_open()) {
                throw runtime_error("cannot open file.");
            }
            fileO.close();
        }
        else {
            file.close();
        }
        rapidcsv::Document documentCSV(pathCSV.string());
        if (static_cast<int>(documentCSV.GetRowCount()) < jsontb.strCount) {
            break;
        }
        csvCount++;
    }
    filesystem::path pathCSV1 = filesystem::current_path() / jsontb.scheme / table / "1.csv";
    filesystem::path pathCSVend = filesystem::current_path() / jsontb.scheme / table / (to_string(csvCount) + ".csv");
    rapidcsv::Document document(pathCSVend.string());
    if (document.GetRowCount() == 0) {
        copyColNames(pathCSV1, pathCSVend);
    }
    ofstream csvFile(pathCSVend, ios::app);
    if (!csvFile.is_open()) {
      throw runtime_error("cannot open file.");
    }
    csvFile << to_string(pk) << ",";
    for (size_t i=1; i < values.size(); i++) {
        if (values[i] == '\'') {
            i++;
            while (values[i]!='\'') {
                csvFile << values[i];
                i++;
            }
            if (values[i+1] != ')') {
                csvFile << ",";
            }
            else {
                csvFile << endl;
            }
        }
    }
    csvFile.close();
    tableLocker(table, jsontb.scheme);
}

void strDelete(string command, TableJson& jsontb) {
    istringstream cmnd(command);
    string mes;
    cmnd >> mes;
    cmnd >> mes;
    if (mes != "FROM") {
        throw runtime_error("invalid data entry.");
    }
    cmnd >> mes;
    string table = mes;
    if (!tableExist(table, jsontb.head)) {
        throw runtime_error("this table is not in the list.");
    }

    if (isLocked(table, jsontb.scheme)) {
        throw runtime_error("file is blocked for edit.");
    }
    cmnd >>mes;
    if (mes != "WHERE") {
        throw runtime_error("invalid data entry.");
    }
    cmnd >> mes;
    string table2;
    string column;
    strSplit(mes, table2, column);
    if (table != table2) {
        throw runtime_error("invalid data entry.");
    }
    if (!columnExist(table, column, jsontb.head)) {
        throw runtime_error("this column is not in the list.");
    }
    cmnd >> mes;
    if (mes != "=") {
        throw runtime_error("invalid data entry.");
    }
    cmnd >> mes;
    if (mes.front() != '\'' || mes.back() != '\'') {
        throw runtime_error("invalid data entry.");
    }
    string value;
    for (size_t i=1; i < mes.size() - 1; i++) {
        value += mes[i];
    }

    tableLocker(table, jsontb.scheme);

    size_t csvCount = 1;
    while (true) {
        filesystem::path pathCSV = filesystem::current_path() / jsontb.scheme / table / (to_string(csvCount) + ".csv");
        ifstream file(pathCSV);
        if (!file.is_open()) {
            break;
        }
        file.close();
        csvCount++;
    }
    for (size_t i = 1; i < csvCount; i++) {
        filesystem::path pathCSV = filesystem::current_path() / jsontb.scheme / table / (to_string(i) + ".csv");
        rapidcsv::Document documentCSV(pathCSV.string());
        int index = documentCSV.GetColumnIdx(column);
        size_t strCount = documentCSV.GetRowCount();
        for (size_t j = 0; j < strCount; j++) {
            if (documentCSV.GetCell<string>(index, j) == value) {
                documentCSV.RemoveRow(j);
                strCount--;
                j--;
            }
        }
        documentCSV.Save(pathCSV.string());
    }
    tableLocker(table, jsontb.scheme);
}

void select(string command, TableJson& jsontb) {
    istringstream cmnd(command);
    string mes;
    cmnd >> mes;
    cmnd >> mes;
    string table1, column1;
    strSplit(mes, table1, column1);
    if (!tableExist(table1, jsontb.head)) {
        throw runtime_error("this table is not in the list.");
    }
    if (!columnExist(table1, column1, jsontb.head)) {
        throw runtime_error("this column is not in the list.");
    }
    cmnd >> mes;
    string table2, column2;
    strSplit(mes, table2, column2);
    if (!tableExist(table2, jsontb.head)) {
        throw runtime_error("this table is not in the list.");
    }
    if (!columnExist(table2, column2, jsontb.head)) {
        throw runtime_error("this column is not in the list.");
    }

    cmnd >> mes;
    if (mes != "FROM") {
        throw runtime_error("invalid data entry.");
    }

    cmnd >> mes;
    if (mes != table1) {
        throw runtime_error("invalid data entry.");
        cout<<mes;
    }

    cmnd >> mes;
    if (mes != table2) {
        throw runtime_error("invalid data entry.");
    }

    cmnd >> mes;
    if (mes == "") {
        crossJoin(table1, column1, table2, column2, jsontb);
    }
    else if (mes != "WHERE") { 
        throw runtime_error("invalid data entry.");
    }
    else {// если есть условия
        cmnd >> mes;      // первый элемент условия
        string tablefilter1, columnfilter1;
        strSplit(mes, tablefilter1, columnfilter1);
        if (!tableExist(tablefilter1, jsontb.head)) {
            throw runtime_error("this table is not in the list.");
        }
        if (!columnExist(tablefilter1, columnfilter1, jsontb.head)) {
            throw runtime_error("this table is not in the list.");
        }

        cmnd >> mes;
        if (mes != "=") {  
            throw runtime_error("invalid data entry.");
        }

        cmnd >> mes;
        if (containsDot(mes) == true) { // второй элемент если содержит точку
            string tablefilter2, columnfilter2;
            strSplit(mes, tablefilter2, columnfilter2);
            if (tablefilter1 != table1) {
                throw runtime_error("invalid data entry.");
            }
            if (tablefilter2 != table2) {
                throw runtime_error("invalid data entry.");
            }
            if (!tableExist(tablefilter2, jsontb.head)) {
                throw runtime_error("this table is not in the list.");
            }
            if (!columnExist(tablefilter2, columnfilter2, jsontb.head)) {
                throw runtime_error("this column is not in the list.");
            }

            cmnd >> mes;
            if (mes != "AND" && mes != "OR") {
                filterTable(jsontb, tablefilter1, tablefilter2, columnfilter1, columnfilter2);
                crossJoinDepend(table1, column1, table2, column2, jsontb);
                size_t icsv = 1;
                while (true) {
                    filesystem::path pathCSVRes1 = filesystem::current_path() / jsontb.scheme / tablefilter1 / ("res_" + to_string(icsv) + ".csv");
                    filesystem::path pathCSVRes2 = filesystem::current_path() / jsontb.scheme / tablefilter2 / ("res_" + to_string(icsv) + ".csv");
                    if (!filesystem::exists(pathCSVRes2) && !filesystem::exists(pathCSVRes1)) 
                        break;
                    if (filesystem::exists(pathCSVRes1)) 
                        filesystem::remove(pathCSVRes1);
                    if (filesystem::exists(pathCSVRes2)) 
                        filesystem::remove(pathCSVRes2);
                    icsv++;
                }
            }
            if (mes == "AND") { // если есть условие И
                cmnd >> mes;
                string tablefilter3, columnfilter3;
                strSplit(mes, tablefilter3, columnfilter3);
                if (tablefilter3 != table1 && tablefilter3 != table2) {
                    throw runtime_error("invalid data entry.");
                }
                if (!tableExist(tablefilter3, jsontb.head)) {
                    throw runtime_error("this table is not in the list.");
                }
                if (!columnExist(tablefilter3, columnfilter3, jsontb.head)) {
                    throw runtime_error("this column is not in the list.");
                }
                cmnd >> mes;
                if (mes != "=") {
                    throw runtime_error("invalid data entry.");
                }
                cmnd >> mes;
                string sfilter;     
                if (mes[0]!='\''){     // если второе условие - значение
                    throw runtime_error("invalid data entry.");
                }
                for (size_t i = 1; i < mes.size()+1; i++) {
                    if (mes[i] != '\'') {
                        sfilter += mes[i];
                    }
                    else break;
                }
                filterAnd(jsontb, tablefilter1, tablefilter2, columnfilter1, columnfilter2, tablefilter3, columnfilter3, sfilter);
                crossJoinDepend(table1, column1, table2, column2, jsontb);
                size_t icsv = 1;
                while (true) {
                    filesystem::path pathCSVRes1 = filesystem::current_path() / jsontb.scheme / tablefilter1 / ("res_" + to_string(icsv) + ".csv");
                    filesystem::path pathCSVRes2 = filesystem::current_path() / jsontb.scheme / tablefilter2 / ("res_" + to_string(icsv) + ".csv");
                    if (!filesystem::exists(pathCSVRes2) && !filesystem::exists(pathCSVRes1)) 
                        break;
                    if (filesystem::exists(pathCSVRes1)) 
                        filesystem::remove(pathCSVRes1);
                    if (filesystem::exists(pathCSVRes2)) 
                        filesystem::remove(pathCSVRes2);
                    icsv++;
                }
            }
            if (mes == "OR") { // ИЛИ
                cmnd >> mes;
                string tablefilter3, columnfilter3;
                strSplit(mes, tablefilter3, columnfilter3);
                if (tablefilter3 != table1 && tablefilter3 != table2) {
                    throw runtime_error("invalid data entry.");
                }
                if (!tableExist(tablefilter3, jsontb.head)) {
                    throw runtime_error("this table is not in the list.");
                }
                if (!columnExist(tablefilter3, columnfilter3, jsontb.head)) {
                    throw runtime_error("this column is not in the list.");
                }
                cmnd >> mes;
                if (mes != "=") {
                    throw runtime_error("invalid data entry.");
                }
                cmnd >> mes;
                string sfilter;
                if (mes[0]!='\''){     // также если второе условие - значепние
                    throw runtime_error("invalid data entry.");
                }
                for (size_t i = 1; i < mes.size()+1; i++) {
                    if (mes[i] != '\'') {
                        sfilter += mes[i];
                    }
                    else break;
                }
                filterOr(jsontb, tablefilter1, tablefilter2, columnfilter1, columnfilter2, tablefilter3, columnfilter3, sfilter);
                crossJoinDepend(table1, column1, table2, column2, jsontb);
                size_t icsv = 1;
                while (true) {
                    filesystem::path pathCSVRes1 = filesystem::current_path() / jsontb.scheme / tablefilter1 / ("res_" + to_string(icsv) + ".csv");
                    filesystem::path pathCSVRes2 = filesystem::current_path() / jsontb.scheme / tablefilter2 / ("res_" + to_string(icsv) + ".csv");
                    if (!filesystem::exists(pathCSVRes2) && !filesystem::exists(pathCSVRes1)) 
                        break;
                    if (filesystem::exists(pathCSVRes1)) 
                        filesystem::remove(pathCSVRes1);
                    if (filesystem::exists(pathCSVRes2)) 
                        filesystem::remove(pathCSVRes2);
                    icsv++;
                }
            }
        }

        else {      // если второй элемент - это значение
            if (tablefilter1 != table1 && tablefilter1 != table2) {
                throw runtime_error("invalid data entry.");
            }
            string sfilter;
            if (mes[0]!= '\''){
                throw runtime_error("invalid data entry.");
            }
            for (size_t i = 1; i < mes.size()+1; i++) {
                if (mes[i] != '\'') {
                sfilter += mes[i];
                }
                else break;
            }
            cmnd >> mes;
            if (mes != "AND" && mes != "OR") {
                if (tablefilter1 == table1) {
                    filterStr(jsontb, tablefilter1, table2, columnfilter1, sfilter);
                    crossJoinDepend(table1, column1, table2, column2, jsontb);
                    size_t icsv = 1;
                    while (true) {
                        filesystem::path pathCSVRes1 = filesystem::current_path() / jsontb.scheme / tablefilter1 / ("res_" + to_string(icsv) + ".csv");
                        filesystem::path pathCSVRes2 = filesystem::current_path() / jsontb.scheme / table2 / ("res_" + to_string(icsv) + ".csv");
                        if (!filesystem::exists(pathCSVRes2) && !filesystem::exists(pathCSVRes1)) 
                            break;
                        if (filesystem::exists(pathCSVRes1)) 
                            filesystem::remove(pathCSVRes1);
                        if (filesystem::exists(pathCSVRes2)) 
                            filesystem::remove(pathCSVRes2);
                        icsv++;
                    }
                }
                if (tablefilter1 == table2) {
                    filterStr(jsontb, tablefilter1, table1, columnfilter1, sfilter);
                    crossJoinDepend(table1, column1, table2, column2, jsontb);
                    size_t icsv = 1;
                    while (true) {
                        filesystem::path pathCSVRes1 = filesystem::current_path() / jsontb.scheme / tablefilter1 / ("res_" + to_string(icsv) + ".csv");
                        filesystem::path pathCSVRes2 = filesystem::current_path() / jsontb.scheme / table1 / ("res_" + to_string(icsv) + ".csv");
                        if (!filesystem::exists(pathCSVRes2) && !filesystem::exists(pathCSVRes1)) 
                            break;
                        if (filesystem::exists(pathCSVRes1)) 
                            filesystem::remove(pathCSVRes1);
                        if (filesystem::exists(pathCSVRes2)) 
                            filesystem::remove(pathCSVRes2);
                        icsv++;
                    }
                }
            }
            if (mes == "AND") {   // И (если второе условие - сравнение данных)
                cmnd >> mes;
                string tablefilter2, columnfilter2;
                strSplit(mes, tablefilter2, columnfilter2);
                if (tablefilter2 != table1) {
                    throw runtime_error("invalid data entry.");
                }
                if (!tableExist(tablefilter2, jsontb.head)) {
                    throw runtime_error("this table is not in the list.");
                }
                if (!columnExist(tablefilter2, columnfilter2, jsontb.head)) {
                    throw runtime_error("this column is not in the list.");
                }
                cmnd >> mes;
                if (mes != "=") {
                    throw runtime_error("invalid data entry.");
                }
                
                cmnd >> mes;
                string tablefilter3, columnfilter3;
                strSplit(mes, tablefilter3, columnfilter3);
                if (tablefilter3 != table2) {
                    throw runtime_error("invalid data entry.");
                }
                if (!tableExist(tablefilter3, jsontb.head)) {
                    throw runtime_error("this table is not in the list.");
                }
                if (!columnExist(tablefilter3, columnfilter3, jsontb.head)) {
                    throw runtime_error("this column is not in the list.");
                }

                filterAnd(jsontb, tablefilter2, tablefilter3, columnfilter2, columnfilter3, tablefilter1, columnfilter1, sfilter);
                crossJoinDepend(table1, column1, table2, column2, jsontb);
                size_t icsv = 1;
                while (true) {
                    filesystem::path pathCSVRes1 = filesystem::current_path() / jsontb.scheme / tablefilter2 / ("res_" + to_string(icsv) + ".csv");
                    filesystem::path pathCSVRes2 = filesystem::current_path() / jsontb.scheme / tablefilter3 / ("res_" + to_string(icsv) + ".csv");
                    if (!filesystem::exists(pathCSVRes2) && !filesystem::exists(pathCSVRes1)) 
                        break;
                    if (filesystem::exists(pathCSVRes1)) 
                        filesystem::remove(pathCSVRes1);
                    if (filesystem::exists(pathCSVRes2)) 
                        filesystem::remove(pathCSVRes2);
                    icsv++;
                }
            }
            if (mes == "OR") { // ИЛИ (также если сравнение данных)
                cmnd >> mes;
                string tablefilter2, columnfilter2;
                strSplit(mes, tablefilter2, columnfilter2);
                if (tablefilter2 != table1) {
                    throw runtime_error("invalid data entry.");
                }
                if (!tableExist(tablefilter2, jsontb.head)) {
                    throw runtime_error("this table is not in the list.");
                }
                if (!columnExist(tablefilter2, columnfilter2, jsontb.head)) {
                    throw runtime_error("this column is not in the list.");
                }
                cmnd >> mes;
                if (mes != "=") {
                    throw runtime_error("invalid data entry.");
                }

                cmnd >> mes;
                string tablefilter3, columnfilter3;
                strSplit(mes, tablefilter3, columnfilter3);
                if (tablefilter3 != table2) {
                    throw runtime_error("invalid data entry.");
                }
                if (!tableExist(tablefilter3, jsontb.head)) {
                    throw runtime_error("this table is not in the list.");
                }
                if (!columnExist(tablefilter3, columnfilter3, jsontb.head)) {
                    throw runtime_error("this column is not in the list.");
                }

                filterOr(jsontb, tablefilter2, tablefilter3, columnfilter2, columnfilter3, tablefilter1, columnfilter1, sfilter);
                crossJoinDepend(table1, column1, table2, column2, jsontb);
                size_t icsv = 1;
                while (true) {
                    filesystem::path pathCSVRes1 = filesystem::current_path() / jsontb.scheme / tablefilter2 / ("res_" + to_string(icsv) + ".csv");
                    filesystem::path pathCSVRes2 = filesystem::current_path() / jsontb.scheme / tablefilter3 / ("res_" + to_string(icsv) + ".csv");
                    if (!filesystem::exists(pathCSVRes2) && !filesystem::exists(pathCSVRes1)) 
                        break;
                    if (filesystem::exists(pathCSVRes1)) 
                        filesystem::remove(pathCSVRes1);
                    if (filesystem::exists(pathCSVRes2)) 
                        filesystem::remove(pathCSVRes2);
                    icsv++;
                }
            }
        }
    }
}
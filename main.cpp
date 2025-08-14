#include <bits/stdc++.h>
#include <mysql.h>
#pragma comment(lib, "libmysql.lib")
using namespace std;

/*
 Hospital Bed & Resource Allocation System (C++ + MySQL)
 - Connects to MySQL DB 'hospital_alloc' and manages beds allocation using priority queue
 - Patients with higher urgency get beds first
 - Tracks resources in a hash map and decrements quantities on allocation
 - Generates simple daily reports from SQL queries
*/

struct Patient { int id; string name; int urgency; string required_resource; };
struct Bed     { int id; string ward; string bed_type; bool is_occupied; };

MYSQL* connect_db(const char* host, const char* user, const char* pass, const char* db, unsigned int port){
    MYSQL* conn = mysql_init(nullptr);
    if(!mysql_real_connect(conn, host, user, pass, db, port, NULL, 0)){
        cerr << "MySQL connect error: " << mysql_error(conn) << endl;
        exit(1);
    }
    return conn;
}

void run_sql(MYSQL* conn, const string &q){
    if(mysql_query(conn, q.c_str())){
        cerr << "SQL error: " << mysql_error(conn) << endl;
    }
}

// Load data helpers
vector<Patient> load_patients(MYSQL* conn){
    vector<Patient> out;
    if(mysql_query(conn, "SELECT id,name,urgency,required_resource FROM patients ORDER BY urgency DESC")){
        cerr << "Error: " << mysql_error(conn) << endl; return out;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))){
        out.push_back({stoi(row[0]), row[1]?row[1]:"", stoi(row[2]), row[3]?row[3]:""});
    }
    mysql_free_result(res);
    return out;
}

vector<Bed> load_available_beds(MYSQL* conn){
    vector<Bed> out;
    if(mysql_query(conn, "SELECT id,ward,bed_type,is_occupied FROM beds WHERE is_occupied = 0")){
        cerr << "Error: " << mysql_error(conn) << endl; return out;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))){
        out.push_back({stoi(row[0]), row[1]?row[1]:"", row[2]?row[2]:"", (row[3] && string(row[3])!=\"0\")});
    }
    mysql_free_result(res);
    return out;
}

unordered_map<string,int> load_resources(MYSQL* conn){
    unordered_map<string,int> mp;
    if(mysql_query(conn, "SELECT name,quantity FROM resources")){
        cerr << "Error: " << mysql_error(conn) << endl; return mp;
    }
    MYSQL_RES* res = mysql_store_result(conn);
    MYSQL_ROW row;
    while((row = mysql_fetch_row(res))){
        mp[row[0]] = stoi(row[1]);
    }
    mysql_free_result(res);
    return mp;
}

// Allocate beds to patients by urgency using a priority queue
void allocate_beds(MYSQL* conn){
    auto patients = load_patients(conn);
    auto beds = load_available_beds(conn);
    auto resources = load_resources(conn);

    // build bed queue (simple FIFO for beds)
    queue<int> bedq;
    for(auto &b: beds) bedq.push(b.id);

    // priority queue for patients by urgency (higher first)
    auto cmp = [](const Patient &a, const Patient &b){ return a.urgency < b.urgency; };
    priority_queue<Patient, vector<Patient>, decltype(cmp)> pq(cmp);
    for(auto &p: patients) pq.push(p);

    vector<pair<int,int>> allocations; // (patient_id, bed_id)
    while(!pq.empty() && !bedq.empty()){
        Patient p = pq.top(); pq.pop();

        // Check resource availability if needed
        if(!p.required_resource.empty()){
            if(resources[p.required_resource] <= 0){
                cout << "Resource '"<<p.required_resource<<"' not available for patient "<<p.name<<" (id "<<p.id<<"). Skipping.\n";
                continue; // skip till next patient
            }else{
                resources[p.required_resource]--; // reserve resource
                // update DB resource quantity
                string q = \"UPDATE resources SET quantity = quantity - 1 WHERE name = '\" + p.required_resource + \"'\";
                run_sql(conn, q);
            }
        }

        int bed_id = bedq.front(); bedq.pop();
        allocations.push_back({p.id, bed_id});

        // mark bed occupied in DB and insert allocation row
        string q1 = \"UPDATE beds SET is_occupied = 1 WHERE id = \" + to_string(bed_id);
        run_sql(conn, q1);
        time_t t = time(nullptr);
        tm *tm = localtime(&t);
        char buf[64]; strftime(buf, sizeof(buf), \"%Y-%m-%d %H:%M:%S\", tm);
        string q2 = \"INSERT INTO allocations(patient_id, bed_id, alloc_time) VALUES(\" + to_string(p.id) + \",\" + to_string(bed_id) + \", '\" + string(buf) + \"')\";
        run_sql(conn, q2);

        cout << \"Allocated bed \"<<bed_id<<\" to patient \"<<p.name<<\" (urgency=\"<<p.urgency<<\")\\n\";
    }

    if(pq.empty()) cout << \"All patients processed or no more patients in waiting list.\\n\";
    if(bedq.empty()) cout << \"No more available beds.\\n\";
}

// Release a bed (manual)
void release_bed(MYSQL* conn){
    int bed_id; cout << \"Enter bed id to release: \"; cin >> bed_id;
    string q1 = \"UPDATE beds SET is_occupied = 0 WHERE id = \" + to_string(bed_id);
    run_sql(conn, q1);
    cout << \"Bed \"<<bed_id<<\" released.\\n\";
}

// Reports
void daily_report(MYSQL* conn){
    cout << \"\\n=== Daily Report ===\\n\";
    // Beds occupancy
    if(mysql_query(conn, \"SELECT COUNT(*) FROM beds WHERE is_occupied=1\")){
        cerr << \"Error: \" << mysql_error(conn) << endl;
    } else {
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);
        cout << \"Occupied beds: \" << (row?row[0]:string(\"0\")) << \"\\n\";
        mysql_free_result(res);
    }
    // Pending patients
    if(mysql_query(conn, \"SELECT COUNT(*) FROM patients WHERE id NOT IN (SELECT patient_id FROM allocations)\")){
        cerr << \"Error: \" << mysql_error(conn) << endl;
    } else {
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);
        cout << \"Unallocated patients: \" << (row?row[0]:string(\"0\")) << \"\\n\";
        mysql_free_result(res);
    }
    // Resource inventory
    if(mysql_query(conn, \"SELECT name, quantity FROM resources\")){
        cerr << \"Error: \" << mysql_error(conn) << endl;
    } else {
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row;
        cout << \"\\nResource inventory:\\n\";
        while((row = mysql_fetch_row(res))){
            cout << \"- \"<<row[0]<<\": \"<<row[1]<<\"\\n\";
        }
        mysql_free_result(res);
    }
}

// Add patient or bed or resource
void add_patient(MYSQL* conn){
    int id, urgency; string name, req;
    cout << \"Patient id: \"; cin >> id; cin.ignore();
    cout << \"Name: \"; getline(cin, name);
    cout << \"Urgency (1-10): \"; cin >> urgency; cin.ignore();
    cout << \"Required resource (blank if none): \"; getline(cin, req);
    string q = \"INSERT INTO patients(id,name,urgency,required_resource) VALUES(\"+to_string(id)+\", '\"+name+\"',\"+to_string(urgency)+\", '\"+req+\"')\";
    run_sql(conn, q);
    cout << \"Inserted patient.\\n\";
}

void add_bed(MYSQL* conn){
    int id; string ward, type;
    cout << \"Bed id: \"; cin >> id; cin.ignore();
    cout << \"Ward: \"; getline(cin, ward);
    cout << \"Bed type: \"; getline(cin, type);
    string q = \"INSERT INTO beds(id,ward,bed_type,is_occupied) VALUES(\"+to_string(id)+\", '\"+ward+\"', '\"+type+\"', 0)\";
    run_sql(conn, q);
    cout << \"Inserted bed.\\n\";
}

void add_resource(MYSQL* conn){
    string name; int qty;
    cout << \"Resource name: \"; getline(cin, name);
    cout << \"Quantity: \"; cin >> qty; cin.ignore();
    string q = \"INSERT INTO resources(name,quantity) VALUES('\"+name+\"',\"+to_string(qty)+\")\";
    run_sql(conn, q);
    cout << \"Added resource.\\n\";
}

int main(){
    // DB credentials - change as needed
    const char* host = \"127.0.0.1\";
    const char* user = \"root\";
    const char* pass = \"password\"; // <-- change this
    const char* db   = \"hospital_alloc\";
    unsigned int port = 3306;

    // Ensure DB exists
    MYSQL* bootstrap = mysql_init(nullptr);
    if(!mysql_real_connect(bootstrap, host, user, pass, NULL, port, NULL, 0)){
        cerr << \"Bootstrap connect error: \" << mysql_error(bootstrap) << endl;
        return 1;
    }
    if(mysql_query(bootstrap, \"CREATE DATABASE IF NOT EXISTS hospital_alloc;\")){
        cerr << \"Create DB error: \" << mysql_error(bootstrap) << endl;
    }
    mysql_close(bootstrap);

    MYSQL* conn = connect_db(host, user, pass, db, port);

    while(true){
        cout << \"\\n=== Hospital Bed & Resource Allocation ===\\n\";
        cout << \"1) Allocate beds to waiting patients\\n2) Release bed\\n3) Add patient\\n4) Add bed\\n5) Add resource\\n6) Daily report\\n7) Exit\\n\";
        cout << \"Choice: \"; int ch; if(!(cin>>ch)) break; cin.ignore();
        if(ch==1) allocate_beds(conn);
        else if(ch==2) release_bed(conn);
        else if(ch==3) add_patient(conn);
        else if(ch==4) add_bed(conn);
        else if(ch==5) add_resource(conn);
        else if(ch==6) daily_report(conn);
        else break;
    }

    mysql_close(conn);
    return 0;
}

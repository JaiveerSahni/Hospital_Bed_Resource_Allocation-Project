-- Hospital Bed & Resource Allocation System schema and sample data (MySQL)
CREATE DATABASE IF NOT EXISTS hospital_alloc;
USE hospital_alloc;

CREATE TABLE IF NOT EXISTS beds (
  id INT PRIMARY KEY,
  ward VARCHAR(50),
  bed_type VARCHAR(50),
  is_occupied TINYINT DEFAULT 0
);

CREATE TABLE IF NOT EXISTS patients (
  id INT PRIMARY KEY,
  name VARCHAR(120),
  urgency INT, -- higher is more urgent (1-10)
  required_resource VARCHAR(120)
);

CREATE TABLE IF NOT EXISTS resources (
  id INT PRIMARY KEY AUTO_INCREMENT,
  name VARCHAR(120),
  quantity INT
);

CREATE TABLE IF NOT EXISTS allocations (
  id INT PRIMARY KEY AUTO_INCREMENT,
  patient_id INT,
  bed_id INT,
  alloc_time DATETIME,
  FOREIGN KEY (patient_id) REFERENCES patients(id) ON DELETE CASCADE,
  FOREIGN KEY (bed_id) REFERENCES beds(id) ON DELETE CASCADE
);

-- Sample beds
INSERT IGNORE INTO beds (id, ward, bed_type, is_occupied) VALUES
(1,'A','ICU',0),
(2,'A','ICU',0),
(3,'B','General',0),
(4,'B','General',0),
(5,'C','Pediatrics',0);

-- Sample patients
INSERT IGNORE INTO patients (id,name,urgency,required_resource) VALUES
(1,'John Doe',9,'Ventilator'),
(2,'Mary Jane',5,'Oxygen Cylinder'),
(3,'Sam Wilson',8,'Ventilator'),
(4,'Lucy Heart',3,''),
(5,'Paul Smith',10,'Ventilator');

-- Sample resources
INSERT IGNORE INTO resources (name,quantity) VALUES
('Ventilator',3),
('Oxygen Cylinder',5),
('ECG Machine',2);

-- Sample allocations (none initially)

# Fast File Transfer Protocol Version 0.15
### Alexis Leclerc 
### 08/22/2024

## Installing and setting up MariaDB on a Linux system.

#### Update and Install mariadb, some systems use 'yum'
```bash
sudo apt update
sudo apt install mariadb-server mariadb-client libmariadb-dev
```

#### Start MariaDB
```bash
sudo systemctl start mariadb
sudo systemctl enable mariadb
```



#### Adding new usernames:
```bash
mysql -u user -p
```

```sql
-- Add a test user (passwords should be hashed in practice)
INSERT INTO users (username, password) VALUES ('testuser', 'testpassword');
```



#### SQL Code to create the database and table
```sql
CREATE DATABASE file_transfer;

USE file_transfer;

CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL
);


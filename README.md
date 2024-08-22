# Fast File Transfer Protocol Version 0.15
### Alexis Leclerc 
### 08/22/2024

## Client.c and Server.c scripts (Fast File Transfer Protocol)

**The purpose of this protocol is to perform fast optimized efficent manipulation of larger than normal datasets.**
* The makefile will create a MariaDB SQL database named "file_transfer", with 1 table called "users" with 3 columns: 'id', 'username', 'password'.

### Connecting and Usage

**The Fast File Transfer Protocol will run by default on port *5475*.**

 For Windows systems it is recomended to use ncat through nmap. [Official nmap Downloads Webpage](https://nmap.org/download#windows)
* To connect with nmap: ncat *domain* 5475

 For Unix systems it is recomended to connect through the Bash Terminal
* To connect with bash terminal: nc *domain* 5475

## Installing and setting up MariaDB on a Linux system.

#### Make sure your server has port '5475' open or portforwarded.

**Update and Install mariadb, some systems use 'yum'**
```bash
sudo apt update
sudo apt install mariadb-server mariadb-client libmariadb-dev
```

**Start MariaDB**
```bash
sudo systemctl start mariadb
sudo systemctl enable mariadb
```

**Adding new usernames:**
```bash
mysql -u user -p
```

**Copy + Paste this into the terminal. Modify the username and password.**
```sql
-- Add a test user (passwords should be hashed in practice)
INSERT INTO users (username, password) VALUES ('testuser', 'testpassword');
```

**SQL Code to create the database and table**
```sql
CREATE DATABASE file_transfer;

USE file_transfer;

CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL
);

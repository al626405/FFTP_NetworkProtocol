# Fast File Transfer Protocol Version 0.2.4
### Alexis Leclerc 
### 08/22/2024 - Most recent update
### Contact
* [Email](mailto:alexisglleclerc@gmail.com)
* [LinkedIn](https://www.linkedin.com/in/alexis-gl-leclerc/)

## Client.c and Server.c scripts (Fast File Transfer Protocol)

**The purpose of this protocol is to perform fast optimized efficent manipulation of larger than normal datasets.**
* The makefile will create a MariaDB SQL database named "file_transfer", with 1 table called "users" with 3 columns: **'id'**, **'username'**, **'password'**.

# Running Client.c & Server.c
```bash
gcc -o Client Client.c -lpthread
./Client
```
### If any errors occur try:
**Client.c:**
```bash
chmod 755 Client.c
```
**Server.c**
```bash
chmod 755 Server.c
```
# Connecting and Usage

### The Fast File Transfer Protocol will run by default on port *5475*.

## For Windows systems it is recomended to use ncat through nmap. 
**[Official nmap Downloads Webpage](https://nmap.org/download#windows).**

**To connect with nmap:**
```bash
ncat host.com 5475
```

## For Unix systems it is recomended to connect through the Bash Terminal.

**To connect with bash terminal:**
```bash
nc host.com 5475
```

## Uploading & Downloading Files

**Uploading a File:**

To upload a file named localfile.txt, you can enter:
```bash
UPLOAD remote_filename.txt
```

Then, use Ctrl+D (or EOF) to send the file contents:
```bash
cat localfile.txt | host.com 5475
```
**Downloading a File:**

To download a file named remote_filename.txt from the server:
```bash
DOWNLOAD remote_filename.txt
```

The server should start sending the file contents, which you can redirect to a local file:
```bash
nc host.com 12345 > downloaded_file.txt
```

## System commands

**List directory contents.**
```bash
ls -l
```

****
```bash
exit
```

**Print the working directory.**
```bash
*pwd
```

**Concatenate and display file content.**
```bash
cat filename.txt
```

**Report file system disk space usage.**
```bash
df -h
```


**Display Linux tasks.**
```bash
top -n 1
```

**Resource/Task Monitor.**
```bash
htop
```

**Search text using patterns.**
```bash
grep 'pattern' filename.txt
```

**Display a line of text..**
```bash
echo "Hello, world!"
```

## Installing and setting up MariaDB on a Linux system.

#### Make sure your server has port '5475' open or portforwarded.

**Update and Install mariadb, some systems use 'yum'**
```bash
sudo apt update
sudo apt install mariadb-server mariadb-Client libmariadb-dev
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

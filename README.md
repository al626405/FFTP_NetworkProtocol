# Fast File Transfer Protocol Version 0.01
### Alexis Leclerc 
### 08/22/2024

## Client.c and Server.c scripts (Fast File Transfer Protocol)

#### The purpose of this protocol is to perform fast optimized efficent manipulation of larger than normal datasets.
* #### The makefile will create a MariaDB SQL database named "file_transfer", with 1 table called "users" with 3 columns: 'id', 'username', 'password'.
* #### READ DB_Setup.md for help with installing and setting up MariaDB. 


## Connecting and Usage

#### The Fast File Transfer Protocol will run by default on port 5475.

#### For Windows systems it is recomended to use ncat through nmap. [Official nmap Downloads Webpage](https://nmap.org/download#windows)
* #### To connect with nmap: ncat *domain* 5475


#### For Unix systems it is recomended to connect through the Bash Terminal
* #### To connect with bash terminal: nc *domain* 5475

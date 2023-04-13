# shell
## Oria Zadok & Yohanan Kling

## assumptions:
##  1. in case of reaching to control flow (if else) when navigating in history we decided to show the just the if part
##      because its meaningless to start the command with fi

## script 1
### ./myshell
### hello: date >> myfile
### hello: cat myfile
### Thu 13 Apr 2023 13:27:57 IDT
### hello: date -u >> myfile
### hello: cat myfile
### Thu 13 Apr 2023 13:27:57 IDT
### Thu 13 Apr 2023 10:28:20 UTC
### hello: wc -l < myfile
### 2
### hello: prompt = hi:
### hi: mkdir mydir
### hi: cd mydir
### hi: pwd
### /home/oriaz/Desktop/task_shell/mydir
### hi: touch file1 file2 file3
### hi: ls
### file1  file2  file3
### hi: !!
### file1  file2  file3
### hi: echo abc xyz
### abc xyz 
### hi: ls
### file1  file2  file3
### hi: echo $?
### 0
### hi: ls no_such_file    
### ls: cannot access 'no_such_file': No such file or directory
### hi: echo $?
### 512
### hi: ls no_such_file 2> file
### hi: ^CYou typed Control-C!
### hi: cat > colors.txt
### blue
### black
### red
### red
### green
### blue
### green
### red
### red
### blue
### hi: cat colors.txt
### blue
### black
### red
### red
### green
### blue
### green
### red
### red
### blue
### hi: cat colors.txt | cat |cat | cat
### blue
### black
### red
### red
### green
### blue
### green
### red
### red
### blue
### hi: sort colors.txt | uniq -c | sort -r | head -3
###       4 red
###       3 blue
###       2 green
### hi: quit
###     

## script 2
### ./myshell
### hello: mkdir newdir
### hello: cd newdir
### hello: pwd
### /home/oriaz/Desktop/task_shell/newdir
### hello: touch file1
### hello: if
### ls | grep file1
### then
### echo file was created
### ls
### else
### echo file1 not found
### fi
### file1
### file was created

## script 3
### ./myshell
### hello: echo 1  
### 1 
### hello: echo 2
### 2 
### hello: echo 3
### 3 
### hello: echo 4
### 4
### hello: ^[[A
### hello: echo 4^[[A
### hello: echo 3
### 3 
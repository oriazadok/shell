# shell
## Oria Zadok & Yohanan Kling

./myshell
hello: date >> myfile
hello: cat myfile
Thu 13 Apr 2023 13:27:57 IDT
hello: date -u >> myfile
hello: cat myfile
Thu 13 Apr 2023 13:27:57 IDT
Thu 13 Apr 2023 10:28:20 UTC
hello: wc -l < myfile
2
hello: prompt = hi:
hi: mkdir mydir
hi: cd mydir
hi: pwd
/home/oriaz/Desktop/task_shell/mydir
hi: touch file1 file2 file3
hi: ls
file1  file2  file3
hi: !!
file1  file2  file3
hi: echo abc xyz
abc xyz 
hi: ls
file1  file2  file3
hi: echo $?
0
hi: ls no_such_file    
ls: cannot access 'no_such_file': No such file or directory
hi: echo $?
512
hi: ls no_such_file 2> file
hi: ^CYou typed Control-C!
hi: cat > colors.txt
blue
black
red
red
green
blue
green
red
red
blue
hi: cat colors.txt
blue
black
red
red
green
blue
green
red
red
blue
hi: cat colors.txt | cat |cat | cat
blue
black
red
red
green
blue
green
red
red
blue
hi: sort colors.txt | uniq -c | sort -r | head -3
      4 red
      3 blue
      2 green
hi: quit